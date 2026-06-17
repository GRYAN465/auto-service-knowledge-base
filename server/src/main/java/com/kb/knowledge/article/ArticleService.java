package com.kb.knowledge.article;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.BusinessException;
import com.kb.common.PageResult;
import com.kb.common.ResultCode;
import com.kb.knowledge.article.dto.ArticleDetailVO;
import com.kb.knowledge.article.dto.ArticleListItemVO;
import com.kb.knowledge.article.dto.ArticleRequest;
import com.kb.knowledge.article.dto.AuditRequest;
import com.kb.knowledge.article.dto.OfflineRequest;
import com.kb.knowledge.article.dto.TagBriefVO;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.entity.KbArticleTag;
import com.kb.knowledge.article.entity.KbArticleVersion;
import com.kb.knowledge.article.entity.KbAttachment;
import com.kb.knowledge.article.entity.KbAuditRecord;
import com.kb.knowledge.article.event.ArticleVectorSyncEvent;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.article.mapper.KbArticleTagMapper;
import com.kb.knowledge.article.mapper.KbArticleVersionMapper;
import com.kb.knowledge.article.mapper.KbAttachmentMapper;
import com.kb.knowledge.article.mapper.KbAuditRecordMapper;
import com.kb.knowledge.category.CategoryService;
import com.kb.knowledge.category.entity.KbCategory;
import com.kb.knowledge.category.mapper.KbCategoryMapper;
import com.kb.knowledge.search.entity.KbViewLog;
import com.kb.knowledge.search.mapper.KbViewLogMapper;
import com.kb.knowledge.tag.entity.KbTag;
import com.kb.knowledge.tag.mapper.KbTagMapper;
import com.kb.security.SecurityUtils;
import com.kb.system.user.entity.SysUser;
import com.kb.system.user.mapper.SysUserMapper;
import org.springframework.context.ApplicationEventPublisher;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.util.StringUtils;

import java.time.LocalDateTime;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * 知识文章业务：CRUD + 版本快照 + 审核/上下线状态机 + 标签挂接。
 * 镜像 {@code TagService}/{@code CategoryService} 的构造注入与 get()-校验风格。
 *
 * <p>状态机（status）：
 * <ul>
 *   <li>create → DRAFT</li>
 *   <li>update：仅 DRAFT/REJECTED 可改，生成新版本快照并 current_version++</li>
 *   <li>submit：DRAFT/REJECTED → PENDING_AUDIT</li>
 *   <li>audit PASS：PENDING_AUDIT → ONLINE（写 online_time）；REJECT → REJECTED</li>
 *   <li>offline：ONLINE → OFFLINE（写 offline_time + reason）</li>
 *   <li>online：OFFLINE → ONLINE</li>
 * </ul>
 */
@Service
public class ArticleService {

    private static final String DRAFT = "DRAFT";
    private static final String PENDING_AUDIT = "PENDING_AUDIT";
    private static final String ONLINE = "ONLINE";
    private static final String OFFLINE = "OFFLINE";
    private static final String REJECTED = "REJECTED";

    private final KbArticleMapper articleMapper;
    private final KbArticleVersionMapper versionMapper;
    private final KbAuditRecordMapper auditRecordMapper;
    private final KbArticleTagMapper articleTagMapper;
    private final KbAttachmentMapper attachmentMapper;
    private final KbCategoryMapper categoryMapper;
    private final KbTagMapper tagMapper;
    private final SysUserMapper userMapper;
    private final KbViewLogMapper viewLogMapper;
    private final CategoryService categoryService;
    private final ApplicationEventPublisher eventPublisher;

    public ArticleService(KbArticleMapper articleMapper, KbArticleVersionMapper versionMapper,
                          KbAuditRecordMapper auditRecordMapper, KbArticleTagMapper articleTagMapper,
                          KbAttachmentMapper attachmentMapper, KbCategoryMapper categoryMapper,
                          KbTagMapper tagMapper, SysUserMapper userMapper, KbViewLogMapper viewLogMapper,
                          CategoryService categoryService, ApplicationEventPublisher eventPublisher) {
        this.articleMapper = articleMapper;
        this.versionMapper = versionMapper;
        this.auditRecordMapper = auditRecordMapper;
        this.articleTagMapper = articleTagMapper;
        this.attachmentMapper = attachmentMapper;
        this.categoryMapper = categoryMapper;
        this.tagMapper = tagMapper;
        this.userMapper = userMapper;
        this.viewLogMapper = viewLogMapper;
        this.categoryService = categoryService;
        this.eventPublisher = eventPublisher;
    }

    // ---------------------------------------------------------------- 查询

    public PageResult<ArticleListItemVO> page(long page, long pageSize, String keyword, Long categoryId,
                                              Long tagId, String status, String knowledgeType) {
        // tagId 条件：先取该标签下的 articleId 集合，空集合直接返回空页。
        List<Long> articleIdsByTag = null;
        if (tagId != null) {
            articleIdsByTag = articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                            .eq(KbArticleTag::getTagId, tagId))
                    .stream().map(KbArticleTag::getArticleId).distinct().toList();
            if (articleIdsByTag.isEmpty()) {
                return new PageResult<>(0, page, pageSize, Collections.emptyList());
            }
        }

        // 分类筛选纳入子分类：所选分类展开为「自身 + 全部子孙」，IN 取代精确等值。
        List<Long> categoryIds = categoryId != null ? categoryService.subtreeIds(categoryId) : null;
        IPage<KbArticle> data = articleMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<KbArticle>lambdaQuery()
                        .like(StringUtils.hasText(keyword), KbArticle::getTitle, keyword)
                        .in(categoryIds != null, KbArticle::getCategoryId, categoryIds)
                        .eq(StringUtils.hasText(status), KbArticle::getStatus, status)
                        .eq(StringUtils.hasText(knowledgeType), KbArticle::getKnowledgeType, knowledgeType)
                        .in(articleIdsByTag != null, KbArticle::getId, articleIdsByTag)
                        .orderByDesc(KbArticle::getUpdateTime)
                        .orderByDesc(KbArticle::getId));

        List<KbArticle> rows = data.getRecords();
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(), toListItems(rows));
    }

    /**
     * 把 {@link KbArticle} 列表富集成 {@link ArticleListItemVO}（批量补 categoryName/authorName/tagNames，避免 N+1）。
     * 供管理后台分页与门户检索（{@code SearchService}）共用。
     */
    public List<ArticleListItemVO> toListItems(List<KbArticle> rows) {
        if (rows == null || rows.isEmpty()) {
            return Collections.emptyList();
        }
        Map<Long, String> categoryNames = categoryNameMap(rows.stream()
                .map(KbArticle::getCategoryId).filter(java.util.Objects::nonNull).collect(Collectors.toSet()));
        Map<Long, String> authorNames = userNameMap(rows.stream()
                .map(KbArticle::getAuthorId).filter(java.util.Objects::nonNull).collect(Collectors.toSet()));
        Map<Long, List<String>> tagNames = tagNamesByArticle(rows.stream()
                .map(KbArticle::getId).collect(Collectors.toSet()));

        return rows.stream().map(a -> {
            ArticleListItemVO vo = new ArticleListItemVO();
            vo.setId(a.getId());
            vo.setTitle(a.getTitle());
            vo.setCategoryId(a.getCategoryId());
            vo.setCategoryName(categoryNames.get(a.getCategoryId()));
            vo.setKnowledgeType(a.getKnowledgeType());
            vo.setStatus(a.getStatus());
            vo.setSource(a.getSource());
            vo.setAuthorId(a.getAuthorId());
            vo.setAuthorName(authorNames.get(a.getAuthorId()));
            vo.setCurrentVersion(a.getCurrentVersion());
            vo.setViewCount(a.getViewCount());
            vo.setUpdateTime(a.getUpdateTime());
            vo.setTagNames(tagNames.getOrDefault(a.getId(), Collections.emptyList()));
            return vo;
        }).toList();
    }

    public ArticleDetailVO detail(Long id) {
        KbArticle a = get(id);
        ArticleDetailVO vo = new ArticleDetailVO();
        vo.setId(a.getId());
        vo.setTitle(a.getTitle());
        vo.setCategoryId(a.getCategoryId());
        vo.setCategoryName(a.getCategoryId() == null ? null
                : categoryNameMap(Set.of(a.getCategoryId())).get(a.getCategoryId()));
        vo.setKnowledgeType(a.getKnowledgeType());
        vo.setSummary(a.getSummary());
        vo.setContent(a.getContent());
        vo.setStatus(a.getStatus());
        vo.setSource(a.getSource());
        vo.setAuthorId(a.getAuthorId());
        vo.setAuthorName(a.getAuthorId() == null ? null
                : userNameMap(Set.of(a.getAuthorId())).get(a.getAuthorId()));
        vo.setCurrentVersion(a.getCurrentVersion());
        vo.setOnlineTime(a.getOnlineTime());
        vo.setOfflineTime(a.getOfflineTime());
        vo.setOfflineReason(a.getOfflineReason());
        vo.setViewCount(a.getViewCount());
        vo.setLikeCount(a.getLikeCount());
        vo.setDislikeCount(a.getDislikeCount());
        vo.setFavoriteCount(a.getFavoriteCount());
        vo.setCommentCount(a.getCommentCount());
        vo.setCreateTime(a.getCreateTime());
        vo.setUpdateTime(a.getUpdateTime());
        vo.setTags(tagsOf(id));
        vo.setAttachments(attachmentMapper.selectList(Wrappers.<KbAttachment>lambdaQuery()
                .eq(KbAttachment::getArticleId, id)
                .orderByAsc(KbAttachment::getId)));
        return vo;
    }

    /**
     * 门户侧查看详情并埋点浏览：返回详情 + view_count++（最小实体自增，镜像 AttachmentService 下载计数）+ 写 kb_view_log。
     */
    @Transactional
    public ArticleDetailVO viewDetail(Long id, String clientIp) {
        ArticleDetailVO vo = detail(id);

        // 原子自增 view_count；用 setSql + 空实体更新（不触发审计字段自动填充），避免浏览把文章顶到管理列表最前。
        long current = vo.getViewCount() == null ? 0L : vo.getViewCount();
        articleMapper.update(null, Wrappers.<KbArticle>lambdaUpdate()
                .setSql("view_count = view_count + 1")
                .eq(KbArticle::getId, id));
        vo.setViewCount(current + 1);

        KbViewLog log = new KbViewLog();
        log.setArticleId(id);
        log.setUserId(SecurityUtils.getUserIdOrNull());
        log.setClientIp(clientIp);
        log.setCreateTime(LocalDateTime.now());
        viewLogMapper.insert(log);
        return vo;
    }

    public KbArticle get(Long id) {
        KbArticle article = articleMapper.selectById(id);
        if (article == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "知识不存在");
        }
        return article;
    }

    public List<KbArticleVersion> versions(Long id) {
        get(id);
        return versionMapper.selectList(Wrappers.<KbArticleVersion>lambdaQuery()
                .eq(KbArticleVersion::getArticleId, id)
                .orderByDesc(KbArticleVersion::getVersionNo));
    }

    // ---------------------------------------------------------------- 写入

    @Transactional
    public KbArticle create(ArticleRequest request) {
        KbArticle article = new KbArticle();
        apply(article, request);
        article.setStatus(DRAFT);
        article.setSource("MANUAL");
        article.setAuthorId(SecurityUtils.getUserIdOrNull());
        article.setCurrentVersion(1);
        article.setViewCount(0L);
        article.setLikeCount(0L);
        article.setDislikeCount(0L);
        article.setFavoriteCount(0L);
        article.setCommentCount(0L);
        articleMapper.insert(article);
        syncTags(article.getId(), request.getTagIds());
        writeVersion(article, "创建草稿");
        return article;
    }

    @Transactional
    public KbArticle update(Long id, ArticleRequest request) {
        KbArticle article = get(id);
        if (!DRAFT.equals(article.getStatus()) && !REJECTED.equals(article.getStatus())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "仅草稿或已驳回的知识可编辑");
        }
        apply(article, request);
        article.setCurrentVersion((article.getCurrentVersion() == null ? 1 : article.getCurrentVersion()) + 1);
        articleMapper.updateById(article);
        syncTags(id, request.getTagIds());
        writeVersion(article, StringUtils.hasText(request.getChangeLog()) ? request.getChangeLog() : "编辑");
        return get(id);
    }

    @Transactional
    public void delete(Long id) {
        get(id);
        articleTagMapper.delete(Wrappers.<KbArticleTag>lambdaQuery().eq(KbArticleTag::getArticleId, id));
        articleMapper.deleteById(id);
        // 删除 → 移除向量
        eventPublisher.publishEvent(new ArticleVectorSyncEvent(id, ArticleVectorSyncEvent.Action.REMOVE));
    }

    @Transactional
    public void submit(Long id) {
        KbArticle article = get(id);
        if (!DRAFT.equals(article.getStatus()) && !REJECTED.equals(article.getStatus())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "仅草稿或已驳回的知识可提交审核");
        }
        article.setStatus(PENDING_AUDIT);
        articleMapper.updateById(article);

        KbAuditRecord record = new KbAuditRecord();
        record.setArticleId(id);
        record.setSubmitBy(SecurityUtils.getUserIdOrNull());
        record.setSubmitTime(LocalDateTime.now());
        auditRecordMapper.insert(record);
    }

    @Transactional
    public void audit(Long id, AuditRequest request) {
        KbArticle article = get(id);
        if (!PENDING_AUDIT.equals(article.getStatus())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "仅待审核的知识可审核");
        }
        boolean pass = "PASS".equalsIgnoreCase(request.getAuditStatus());
        boolean reject = "REJECT".equalsIgnoreCase(request.getAuditStatus());
        if (!pass && !reject) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "审核结论须为 PASS 或 REJECT");
        }
        LocalDateTime now = LocalDateTime.now();
        if (pass) {
            article.setStatus(ONLINE);
            article.setOnlineTime(now);
        } else {
            article.setStatus(REJECTED);
        }
        articleMapper.updateById(article);

        KbAuditRecord record = new KbAuditRecord();
        record.setArticleId(id);
        record.setAuditorId(SecurityUtils.getUserIdOrNull());
        record.setAuditStatus(pass ? "PASS" : "REJECT");
        record.setAuditOpinion(request.getOpinion());
        record.setAuditTime(now);
        auditRecordMapper.insert(record);

        if (pass) {
            // 审核通过即上线 → 触发向量化（事务提交后执行，失败不回滚业务）
            eventPublisher.publishEvent(new ArticleVectorSyncEvent(id, ArticleVectorSyncEvent.Action.INDEX));
        }
    }

    @Transactional
    public void offline(Long id, OfflineRequest request) {
        KbArticle article = get(id);
        if (!ONLINE.equals(article.getStatus())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "仅已上线的知识可下线");
        }
        article.setStatus(OFFLINE);
        article.setOfflineTime(LocalDateTime.now());
        article.setOfflineReason(request == null ? null : request.getReason());
        articleMapper.updateById(article);
        // 下线 → 移除向量（offline 文章不应再被问答命中）
        eventPublisher.publishEvent(new ArticleVectorSyncEvent(id, ArticleVectorSyncEvent.Action.REMOVE));
    }

    @Transactional
    public void online(Long id) {
        KbArticle article = get(id);
        if (!OFFLINE.equals(article.getStatus())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "仅已下线的知识可重新上线");
        }
        article.setStatus(ONLINE);
        article.setOnlineTime(LocalDateTime.now());
        articleMapper.updateById(article);
        // 重新上线 → 触发向量化
        eventPublisher.publishEvent(new ArticleVectorSyncEvent(id, ArticleVectorSyncEvent.Action.INDEX));
    }

    // ---------------------------------------------------------------- 私有

    private void apply(KbArticle article, ArticleRequest request) {
        article.setTitle(request.getTitle());
        article.setCategoryId(request.getCategoryId());
        article.setKnowledgeType(request.getKnowledgeType());
        article.setSummary(request.getSummary());
        article.setContent(request.getContent());
    }

    /** 删旧关系再插新；校验 tag 存在。 */
    private void syncTags(Long articleId, List<Long> tagIds) {
        articleTagMapper.delete(Wrappers.<KbArticleTag>lambdaQuery().eq(KbArticleTag::getArticleId, articleId));
        if (tagIds == null || tagIds.isEmpty()) {
            return;
        }
        for (Long tagId : tagIds.stream().distinct().toList()) {
            if (tagMapper.selectById(tagId) == null) {
                throw new BusinessException(ResultCode.PARAM_ERROR, "标签不存在：" + tagId);
            }
            KbArticleTag relation = new KbArticleTag();
            relation.setArticleId(articleId);
            relation.setTagId(tagId);
            articleTagMapper.insert(relation);
        }
    }

    private void writeVersion(KbArticle article, String changeLog) {
        KbArticleVersion version = new KbArticleVersion();
        version.setArticleId(article.getId());
        version.setVersionNo(article.getCurrentVersion());
        version.setTitle(article.getTitle());
        version.setContent(article.getContent());
        version.setEditorId(SecurityUtils.getUserIdOrNull());
        version.setChangeLog(changeLog);
        version.setCreateTime(LocalDateTime.now());
        versionMapper.insert(version);
    }

    private List<TagBriefVO> tagsOf(Long articleId) {
        List<Long> tagIds = articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                        .eq(KbArticleTag::getArticleId, articleId))
                .stream().map(KbArticleTag::getTagId).toList();
        if (tagIds.isEmpty()) {
            return Collections.emptyList();
        }
        return tagMapper.selectBatchIds(tagIds).stream()
                .map(t -> new TagBriefVO(t.getId(), t.getName(), t.getColor()))
                .toList();
    }

    private Map<Long, String> categoryNameMap(Set<Long> ids) {
        if (ids == null || ids.isEmpty()) {
            return Collections.emptyMap();
        }
        return categoryMapper.selectBatchIds(ids).stream()
                .collect(Collectors.toMap(KbCategory::getId, KbCategory::getName, (a, b) -> a));
    }

    private Map<Long, String> userNameMap(Set<Long> ids) {
        if (ids == null || ids.isEmpty()) {
            return Collections.emptyMap();
        }
        return userMapper.selectBatchIds(ids).stream().collect(Collectors.toMap(SysUser::getId,
                u -> StringUtils.hasText(u.getRealName()) ? u.getRealName() : u.getUsername(), (a, b) -> a));
    }

    private Map<Long, List<String>> tagNamesByArticle(Set<Long> articleIds) {
        if (articleIds == null || articleIds.isEmpty()) {
            return Collections.emptyMap();
        }
        List<KbArticleTag> relations = articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                .in(KbArticleTag::getArticleId, articleIds));
        if (relations.isEmpty()) {
            return Collections.emptyMap();
        }
        Map<Long, String> tagNameById = tagMapper.selectBatchIds(relations.stream()
                        .map(KbArticleTag::getTagId).collect(Collectors.toSet())).stream()
                .collect(Collectors.toMap(KbTag::getId, KbTag::getName, (a, b) -> a));
        return relations.stream().collect(Collectors.groupingBy(KbArticleTag::getArticleId,
                Collectors.mapping(r -> tagNameById.getOrDefault(r.getTagId(), null),
                        Collectors.filtering(java.util.Objects::nonNull, Collectors.toList()))));
    }
}
