package com.kb.ai;

import com.kb.knowledge.article.event.ArticleVectorSyncEvent;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;
import org.springframework.transaction.event.TransactionPhase;
import org.springframework.transaction.event.TransactionalEventListener;

/**
 * 文章向量同步监听器：在文章上下线事务提交后（{@code AFTER_COMMIT}）同步 Chroma 索引。
 * 索引失败已在 {@link VectorIndexService} 内吞掉并落 {@code kb_vector_index.status}，这里再兜一层，
 * 确保监听器异常绝不外溢、不回滚已提交的文章状态。
 */
@Slf4j
@Component
public class ArticleIndexListener {

    private final VectorIndexService vectorIndexService;

    public ArticleIndexListener(VectorIndexService vectorIndexService) {
        this.vectorIndexService = vectorIndexService;
    }

    @TransactionalEventListener(phase = TransactionPhase.AFTER_COMMIT)
    public void onArticleVectorSync(ArticleVectorSyncEvent event) {
        try {
            if (event.getAction() == ArticleVectorSyncEvent.Action.REMOVE) {
                vectorIndexService.removeArticle(event.getArticleId());
            } else {
                vectorIndexService.reindexArticle(event.getArticleId());
            }
        } catch (Exception e) {
            log.warn("文章 {} 向量同步异常（已忽略）：{}", event.getArticleId(), e.toString());
        }
    }
}
