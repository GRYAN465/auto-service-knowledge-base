package com.kb.ai;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.ai.dto.AiCitation;
import com.kb.ai.dto.AiQaResponse;
import com.kb.ai.dto.CitationVO;
import com.kb.ai.dto.FeedbackRequest;
import com.kb.ai.dto.QaMessageVO;
import com.kb.ai.dto.QaSessionVO;
import com.kb.ai.dto.QaVO;
import com.kb.ai.entity.QaFeedback;
import com.kb.ai.entity.QaMessage;
import com.kb.ai.entity.QaReference;
import com.kb.ai.entity.QaSession;
import com.kb.ai.mapper.QaFeedbackMapper;
import com.kb.ai.mapper.QaMessageMapper;
import com.kb.ai.mapper.QaReferenceMapper;
import com.kb.ai.mapper.QaSessionMapper;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.category.CategoryService;
import com.kb.security.SecurityUtils;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.math.BigDecimal;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * 智能问答编排：调用 Python {@code /ai/qa} 做检索增强问答，回填引用真实标题，落库会话/消息/引用，
 * 并处理反馈与历史读取。
 *
 * <p>权限与上线范围由本层把控：内部问答默认 {@code allowedArticleIds=null}（Chroma 只含在线知识）；
 * 带 {@code categoryId} 时收敛为「该分类子树 ∩ 在线」的 id 列表（复用 {@link CategoryService#subtreeIds}）。
 *
 * <p>与 {@code VectorIndexService} 同理，HTTP 调用在事务外进行、再顺序落库（autocommit），
 * 不把 QA 的网络延迟圈进数据库事务。Python 不可用时问答上抛友好错误（区别于索引同步的「吞」）。
 */
@Slf4j
@Service
public class QaService {

    private static final String ONLINE = "ONLINE";
    private static final int DEFAULT_TOP_K = 5;
    private static final int MAX_TOP_K = 20;
    private static final int TITLE_MAX = 50;

    private final AiClient aiClient;
    private final QaSessionMapper sessionMapper;
    private final QaMessageMapper messageMapper;
    private final QaReferenceMapper referenceMapper;
    private final QaFeedbackMapper feedbackMapper;
    private final KbArticleMapper articleMapper;
    private final CategoryService categoryService;

    public QaService(AiClient aiClient, QaSessionMapper sessionMapper, QaMessageMapper messageMapper,
                     QaReferenceMapper referenceMapper, QaFeedbackMapper feedbackMapper,
                     KbArticleMapper articleMapper, CategoryService categoryService) {
        this.aiClient = aiClient;
        this.sessionMapper = sessionMapper;
        this.messageMapper = messageMapper;
        this.referenceMapper = referenceMapper;
        this.feedbackMapper = feedbackMapper;
        this.articleMapper = articleMapper;
        this.categoryService = categoryService;
    }

    // ---------------------------------------------------------------- 问答

    /** 内部问答（受 JWT/权限保护）：问答 + 标题回填 + 落库会话/消息/引用。 */
    public QaVO ask(String question, Integer topK, Long categoryId, Long sessionId, String knowledgeType) {
        Long userId = SecurityUtils.getUserIdOrNull();
        int k = normalizeTopK(topK);
        QaVO vo = answerCore(question, k, allowedIdsFor(categoryId), knowledgeType);

        QaSession session = resolveSession(sessionId, userId, question);
        QaMessage msg = saveMessage(session.getId(), userId, question, vo, k);
        saveReferences(msg.getId(), vo.getCitations());

        vo.setSessionId(session.getId());
        vo.setMessageId(msg.getId());
        return vo;
    }

    /** 对外开放问答（HMAC 鉴权由 OpenApiController 完成）：复用核心，不限范围，不落会话。 */
    public QaVO openQa(String question, Integer topK, String knowledgeType) {
        return answerCore(question, normalizeTopK(topK), null, knowledgeType);
    }

    /** 调 Python 问答并回填引用标题，组装 QaVO（不含 session/message id）。 */
    private QaVO answerCore(String question, int topK, List<Long> allowedIds, String knowledgeType) {
        AiQaResponse resp;
        try {
            resp = aiClient.qa(question, knowledgeType != null ? knowledgeType : "", topK, allowedIds);
        } catch (Exception e) {
            log.warn("问答服务调用失败：{}", e.toString());
            throw new BusinessException(ResultCode.SERVER_ERROR, "智能问答服务暂不可用，请稍后再试");
        }
        if (resp == null) {
            throw new BusinessException(ResultCode.SERVER_ERROR, "智能问答服务无响应");
        }
        QaVO vo = new QaVO();
        vo.setAnswer(resp.getAnswer());
        vo.setMode(resp.getMode());
        vo.setCitations(backfillCitations(resp.getCitations()));
        return vo;
    }

    /** categoryId 为空→null（全体在线）；否则取「子树 ∩ 在线」id（可能为空列表→Python no_hit）。 */
    private List<Long> allowedIdsFor(Long categoryId) {
        if (categoryId == null) {
            return null;
        }
        List<Long> subtree = categoryService.subtreeIds(categoryId);
        if (subtree == null || subtree.isEmpty()) {
            return List.of();
        }
        return articleMapper.selectList(Wrappers.<KbArticle>lambdaQuery()
                        .select(KbArticle::getId)
                        .eq(KbArticle::getStatus, ONLINE)
                        .in(KbArticle::getCategoryId, subtree))
                .stream().map(KbArticle::getId).toList();
    }

    /** 用 kb_article.title 覆盖 Python 占位标题（缺失的文章保留占位）。 */
    private List<CitationVO> backfillCitations(List<AiCitation> raw) {
        if (raw == null || raw.isEmpty()) {
            return List.of();
        }
        Set<Long> ids = raw.stream().map(AiCitation::getArticleId)
                .filter(Objects::nonNull).collect(Collectors.toSet());
        Map<Long, String> titleById = ids.isEmpty() ? Map.of()
                : articleMapper.selectBatchIds(ids).stream()
                        .collect(Collectors.toMap(KbArticle::getId, KbArticle::getTitle, (a, b) -> a));
        List<CitationVO> out = new ArrayList<>(raw.size());
        for (AiCitation c : raw) {
            CitationVO vo = new CitationVO();
            vo.setArticleId(c.getArticleId());
            vo.setTitle(titleById.getOrDefault(c.getArticleId(), c.getTitle()));
            vo.setScore(c.getScore());
            vo.setSnippet(c.getSnippet());
            out.add(vo);
        }
        return out;
    }

    // ---------------------------------------------------------------- 反馈

    /** 点赞/点踩：每用户每消息一条，存在即更新（避开软删唯一名冲突，显式 select 后 insert/update）。 */
    public void feedback(FeedbackRequest req) {
        Long userId = SecurityUtils.getUserIdOrNull();
        if (messageMapper.selectById(req.getMessageId()) == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "消息不存在");
        }
        LocalDateTime now = LocalDateTime.now();
        QaFeedback existing = feedbackMapper.selectOne(Wrappers.<QaFeedback>lambdaQuery()
                .eq(QaFeedback::getMessageId, req.getMessageId())
                .eq(QaFeedback::getUserId, userId));
        if (existing == null) {
            QaFeedback fb = new QaFeedback();
            fb.setMessageId(req.getMessageId());
            fb.setUserId(userId);
            fb.setHelpful(req.getHelpful());
            fb.setReason(truncate(req.getReason(), 250));
            fb.setCreateTime(now);
            fb.setUpdateTime(now);
            feedbackMapper.insert(fb);
        } else {
            existing.setHelpful(req.getHelpful());
            existing.setReason(truncate(req.getReason(), 250));
            existing.setUpdateTime(now);
            feedbackMapper.updateById(existing);
        }
    }

    // ---------------------------------------------------------------- 历史

    /** 当前用户的会话列表（按最近更新倒序）。 */
    public List<QaSessionVO> listSessions() {
        Long userId = SecurityUtils.getUserIdOrNull();
        return sessionMapper.selectList(Wrappers.<QaSession>lambdaQuery()
                        .eq(QaSession::getUserId, userId)
                        .orderByDesc(QaSession::getUpdateTime)
                        .orderByDesc(QaSession::getId))
                .stream().map(s -> {
                    QaSessionVO vo = new QaSessionVO();
                    vo.setId(s.getId());
                    vo.setTitle(s.getTitle());
                    vo.setMessageCount(s.getMessageCount());
                    vo.setCreateTime(s.getCreateTime());
                    vo.setUpdateTime(s.getUpdateTime());
                    return vo;
                }).toList();
    }

    /** 某会话的历史消息（限本人），按时间正序，含每条答案的引用。 */
    public List<QaMessageVO> listMessages(Long sessionId) {
        Long userId = SecurityUtils.getUserIdOrNull();
        QaSession session = sessionMapper.selectById(sessionId);
        if (session == null || (userId != null && !userId.equals(session.getUserId()))) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "会话不存在");
        }
        List<QaMessage> messages = messageMapper.selectList(Wrappers.<QaMessage>lambdaQuery()
                .eq(QaMessage::getSessionId, sessionId)
                .orderByAsc(QaMessage::getId));
        if (messages.isEmpty()) {
            return List.of();
        }
        List<Long> msgIds = messages.stream().map(QaMessage::getId).toList();
        Map<Long, List<CitationVO>> refsByMsg = referenceMapper.selectList(Wrappers.<QaReference>lambdaQuery()
                        .in(QaReference::getMessageId, msgIds)
                        .orderByAsc(QaReference::getSort)
                        .orderByAsc(QaReference::getId))
                .stream().collect(Collectors.groupingBy(QaReference::getMessageId,
                        Collectors.mapping(this::toCitationVO, Collectors.toList())));
        return messages.stream().map(m -> {
            QaMessageVO vo = new QaMessageVO();
            vo.setId(m.getId());
            vo.setSessionId(m.getSessionId());
            vo.setQuestion(m.getQuestion());
            vo.setAnswer(m.getAnswer());
            vo.setMode(m.getMode());
            vo.setTopK(m.getTopK());
            vo.setCreateTime(m.getCreateTime());
            vo.setCitations(refsByMsg.getOrDefault(m.getId(), List.of()));
            return vo;
        }).toList();
    }

    // ---------------------------------------------------------------- 私有：落库

    /** 取已有会话（校验属主、消息数 +1）或新建会话（标题=首问截断）。 */
    private QaSession resolveSession(Long sessionId, Long userId, String firstQuestion) {
        if (sessionId != null) {
            QaSession s = sessionMapper.selectById(sessionId);
            if (s == null || (userId != null && !userId.equals(s.getUserId()))) {
                throw new BusinessException(ResultCode.PARAM_ERROR, "会话不存在");
            }
            s.setMessageCount((s.getMessageCount() == null ? 0 : s.getMessageCount()) + 1);
            sessionMapper.updateById(s);
            return s;
        }
        QaSession s = new QaSession();
        s.setUserId(userId);
        s.setTitle(truncate(firstQuestion, TITLE_MAX));
        s.setMessageCount(1);
        sessionMapper.insert(s);   // create_by/create_time/update_time/deleted 由 MyMetaObjectHandler 填充
        return s;
    }

    private QaMessage saveMessage(Long sessionId, Long userId, String question, QaVO vo, int topK) {
        QaMessage msg = new QaMessage();
        msg.setSessionId(sessionId);
        msg.setUserId(userId);
        msg.setQuestion(question);
        msg.setAnswer(vo.getAnswer());
        msg.setMode(vo.getMode());
        msg.setTopK(topK);
        msg.setCreateTime(LocalDateTime.now());
        messageMapper.insert(msg);
        return msg;
    }

    private void saveReferences(Long messageId, List<CitationVO> citations) {
        if (citations == null || citations.isEmpty()) {
            return;
        }
        LocalDateTime now = LocalDateTime.now();
        int sort = 0;
        for (CitationVO c : citations) {
            QaReference ref = new QaReference();
            ref.setMessageId(messageId);
            ref.setArticleId(c.getArticleId());
            ref.setTitle(truncate(c.getTitle(), 250));
            ref.setScore(c.getScore() != null ? BigDecimal.valueOf(c.getScore()) : null);
            ref.setSnippet(truncate(c.getSnippet(), 500));
            ref.setSort(sort++);
            ref.setCreateTime(now);
            referenceMapper.insert(ref);
        }
    }

    private CitationVO toCitationVO(QaReference r) {
        CitationVO vo = new CitationVO();
        vo.setArticleId(r.getArticleId());
        vo.setTitle(r.getTitle());
        vo.setScore(r.getScore() != null ? r.getScore().doubleValue() : null);
        vo.setSnippet(r.getSnippet());
        return vo;
    }

    private static int normalizeTopK(Integer topK) {
        if (topK == null || topK <= 0) {
            return DEFAULT_TOP_K;
        }
        return Math.min(topK, MAX_TOP_K);
    }

    private static String truncate(String s, int max) {
        if (s == null) {
            return null;
        }
        return s.length() <= max ? s : s.substring(0, max);
    }
}
