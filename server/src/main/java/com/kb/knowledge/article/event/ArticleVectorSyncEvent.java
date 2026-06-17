package com.kb.knowledge.article.event;

import lombok.Getter;

/**
 * 文章向量同步事件：上线/重新上线 → {@code INDEX}；下线/删除 → {@code REMOVE}。
 *
 * <p>{@code ArticleService} 在事务内发布，AI 编排层（{@code ArticleIndexListener}）在事务提交后
 * （{@code AFTER_COMMIT}）消费，索引失败不回滚业务。事件放在 {@code knowledge.article.event} 包，
 * 使 knowledge 模块不反向依赖 ai 模块。
 */
@Getter
public class ArticleVectorSyncEvent {

    public enum Action {
        INDEX,
        REMOVE
    }

    private final Long articleId;
    private final Action action;

    public ArticleVectorSyncEvent(Long articleId, Action action) {
        this.articleId = articleId;
        this.action = action;
    }
}
