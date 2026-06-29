package com.kb.ai;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.ai.dto.AiIndexResponse;
import com.kb.ai.entity.KbChunk;
import com.kb.ai.entity.KbVectorIndex;
import com.kb.ai.mapper.KbChunkMapper;
import com.kb.ai.mapper.KbVectorIndexMapper;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.time.LocalDateTime;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;

/**
 * 向量索引编排 + 记账：把知识正文（标题 + 摘要 + 正文去 HTML）送 Python 向量化入库，并维护
 * {@code kb_chunk}（来源文本）与 {@code kb_vector_index}（每篇状态）。
 *
 * <p>设计要点：
 * <ul>
 *   <li><b>不开启事务</b>：本类方法由 {@code AFTER_COMMIT} 监听器或 admin 请求线程调用，此时无环绕事务；
 *       HTTP 调用（最长含 LLM 不参与，但向量化仍可能秒级）在事务外进行，避免长时间占用连接。
 *       {@code kb_chunk}/{@code kb_vector_index} 为可重建的派生数据，逐条写入（autocommit）即可。</li>
 *   <li><b>失败不外溢</b>：Python 不可用/抖动时记 {@code status=FAILED} 并吞掉异常，绝不影响文章上下线主流程。</li>
 *   <li><b>正文为主</b>：{@code kb_chunk} 存提交的整段来源文本（单行），Python 内部按长度+重叠切分的真实块数
 *       记入 {@code kb_vector_index.chunk_count}。</li>
 * </ul>
 */
@Slf4j
@Service
public class VectorIndexService {

    private static final String ONLINE = "ONLINE";

    private static final String STATUS_INDEXED = "INDEXED";
    private static final String STATUS_REMOVED = "REMOVED";
    private static final String STATUS_FAILED = "FAILED";
    private static final String STATUS_EMPTY = "EMPTY";

    private static final Pattern HTML_TAG = Pattern.compile("<[^>]+>");

    private final AiClient aiClient;
    private final KbChunkMapper chunkMapper;
    private final KbVectorIndexMapper vectorIndexMapper;
    private final KbArticleMapper articleMapper;

    public VectorIndexService(AiClient aiClient, KbChunkMapper chunkMapper,
                              KbVectorIndexMapper vectorIndexMapper, KbArticleMapper articleMapper) {
        this.aiClient = aiClient;
        this.chunkMapper = chunkMapper;
        this.vectorIndexMapper = vectorIndexMapper;
        this.articleMapper = articleMapper;
    }

    /** 按 id 取文章后重建索引；文章不存在（已删）则跳过。供监听器调用。 */
    public String reindexArticle(Long articleId) {
        KbArticle article = articleMapper.selectById(articleId);
        if (article == null) {
            log.warn("文章 {} 不存在，跳过向量化", articleId);
            return "SKIP";
        }
        return reindexArticle(article);
    }

    /**
     * 重建某篇知识的向量索引：拼正文 → 送 Python 入库（整篇覆盖）→ 重写 kb_chunk + upsert kb_vector_index。
     * 正文为空记 EMPTY；Python 调用失败记 FAILED。返回最终状态码。
     */
    public String reindexArticle(KbArticle article) {
        Long articleId = article.getId();
        String text = buildIndexText(article);
        if (!StringUtils.hasText(text)) {
            safeRemoveVectors(articleId, article.getKnowledgeType());
            deleteChunks(articleId);
            saveStatus(articleId, STATUS_EMPTY, 0, 0, null, null);
            log.info("文章 {} 无可索引正文，记 EMPTY", articleId);
            return STATUS_EMPTY;
        }
        try {
            AiIndexResponse resp = aiClient.index(articleId, article.getKnowledgeType(), List.of(text));
            int chunkCount = resp != null && resp.getChunkCount() != null ? resp.getChunkCount() : 0;
            int dim = resp != null && resp.getDim() != null ? resp.getDim() : 0;
            rewriteChunk(articleId, text);
            saveStatus(articleId, STATUS_INDEXED, chunkCount, dim, null, LocalDateTime.now());
            log.info("文章 {} 向量化完成：chunk_count={} dim={}", articleId, chunkCount, dim);
            return STATUS_INDEXED;
        } catch (Exception e) {
            saveStatus(articleId, STATUS_FAILED, null, null, truncate(e.getMessage(), 500), null);
            log.warn("文章 {} 向量化失败（不影响业务）：{}", articleId, e.toString());
            return STATUS_FAILED;
        }
    }

    /** 移除某篇知识的全部向量块与来源文本，记 REMOVED。Python 调用失败也吞掉（best-effort）。 */
    public void removeArticle(Long articleId) {
        String kt = articleMapper.selectById(articleId) != null
                ? articleMapper.selectById(articleId).getKnowledgeType() : null;
        safeRemoveVectors(articleId, kt);
        deleteChunks(articleId);
        saveStatus(articleId, STATUS_REMOVED, 0, 0, null, null);
        log.info("文章 {} 向量已移除", articleId);
    }

    /** 重建全部 ONLINE 知识的索引（admin 回填历史在线知识）。返回 {total,indexed,failed,empty}。 */
    public Map<String, Integer> reindexAllOnline() {
        List<KbArticle> online = articleMapper.selectList(Wrappers.<KbArticle>lambdaQuery()
                .eq(KbArticle::getStatus, ONLINE));
        int indexed = 0, failed = 0, empty = 0;
        for (KbArticle a : online) {
            String status = reindexArticle(a);
            if (STATUS_INDEXED.equals(status)) {
                indexed++;
            } else if (STATUS_EMPTY.equals(status)) {
                empty++;
            } else {
                failed++;
            }
        }
        Map<String, Integer> result = new LinkedHashMap<>();
        result.put("total", online.size());
        result.put("indexed", indexed);
        result.put("failed", failed);
        result.put("empty", empty);
        log.info("批量重建索引完成：{}", result);
        return result;
    }

    // ---------------------------------------------------------------- 私有

    /** 拼装送入向量化的文本：标题 + 摘要 + 正文（去 HTML）。 */
    private String buildIndexText(KbArticle a) {
        StringBuilder sb = new StringBuilder();
        if (StringUtils.hasText(a.getTitle())) {
            sb.append(a.getTitle().trim()).append('\n');
        }
        if (StringUtils.hasText(a.getSummary())) {
            sb.append(a.getSummary().trim()).append('\n');
        }
        String body = htmlToText(a.getContent());
        if (StringUtils.hasText(body)) {
            sb.append(body);
        }
        return sb.toString().trim();
    }

    private void rewriteChunk(Long articleId, String text) {
        deleteChunks(articleId);
        KbChunk chunk = new KbChunk();
        chunk.setArticleId(articleId);
        chunk.setSeq(0);
        chunk.setContent(text);
        chunk.setCharLen(text.length());
        chunk.setCreateTime(LocalDateTime.now());
        chunkMapper.insert(chunk);
    }

    private void deleteChunks(Long articleId) {
        chunkMapper.delete(Wrappers.<KbChunk>lambdaQuery().eq(KbChunk::getArticleId, articleId));
    }

    private void safeRemoveVectors(Long articleId, String knowledgeType) {
        try {
            aiClient.removeIndex(articleId, knowledgeType != null ? knowledgeType : "");
        } catch (Exception e) {
            log.warn("移除文章 {} 向量失败（不影响业务）：{}", articleId, e.toString());
        }
    }

    /** upsert kb_vector_index：按 article_id 唯一行更新或插入；chunkCount/dim 为 null 时保留原值。 */
    private void saveStatus(Long articleId, String status, Integer chunkCount, Integer dim,
                            String errorMsg, LocalDateTime indexedTime) {
        KbVectorIndex existing = vectorIndexMapper.selectOne(Wrappers.<KbVectorIndex>lambdaQuery()
                .eq(KbVectorIndex::getArticleId, articleId));
        LocalDateTime now = LocalDateTime.now();
        KbVectorIndex row = existing != null ? existing : new KbVectorIndex();
        row.setArticleId(articleId);
        row.setStatus(status);
        if (chunkCount != null) {
            row.setChunkCount(chunkCount);
        }
        if (dim != null) {
            row.setDim(dim);
        }
        row.setErrorMsg(errorMsg);
        if (indexedTime != null) {
            row.setIndexedTime(indexedTime);
        }
        row.setUpdateTime(now);
        if (existing == null) {
            if (row.getChunkCount() == null) {
                row.setChunkCount(0);
            }
            if (row.getDim() == null) {
                row.setDim(0);
            }
            row.setCreateTime(now);
            vectorIndexMapper.insert(row);
        } else {
            vectorIndexMapper.updateById(row);
        }
    }

    /** 去 HTML 标签 + 反转义 + 压缩空白，足够喂 embedding。 */
    private static String htmlToText(String html) {
        if (!StringUtils.hasText(html)) {
            return "";
        }
        String text = html.replaceAll("(?is)<(script|style)[^>]*>.*?</\\1>", " ");  // 去脚本/样式
        text = text.replaceAll("(?i)<\\s*br\\s*/?>", "\n");
        text = text.replaceAll("(?i)</(p|div|li|h[1-6]|tr)\\s*>", "\n");
        text = HTML_TAG.matcher(text).replaceAll(" ");                              // 去剩余标签
        text = unescapeHtml(text);
        text = text.replaceAll("[\\t\\x0B\\f\\r ]+", " ");                          // 行内空白压成单空格
        text = text.replaceAll(" *\\n *", "\n");                                   // 修剪换行两侧空白
        text = text.replaceAll("\\n{3,}", "\n\n");                                 // 最多保留一个空行
        return text.trim();
    }

    private static String unescapeHtml(String s) {
        return s.replace("&nbsp;", " ")
                .replace("&lt;", "<")
                .replace("&gt;", ">")
                .replace("&quot;", "\"")
                .replace("&#39;", "'")
                .replace("&apos;", "'")
                .replace("&amp;", "&");   // &amp; 放最后，避免二次解码
    }

    private static String truncate(String s, int max) {
        if (s == null) {
            return null;
        }
        return s.length() <= max ? s : s.substring(0, max);
    }
}
