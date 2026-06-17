package com.kb.statistics.dto;

import lombok.Data;

/**
 * 知识计数列汇总（kb_article 的 SUM 聚合）：浏览/点赞/收藏/评论总量。
 * 仅供 {@link com.kb.statistics.StatisticsService} 装配 {@link OverviewVO} 使用。
 */
@Data
public class ArticleTotalsVO {

    private Long viewTotal;
    private Long likeTotal;
    private Long favoriteTotal;
    private Long commentTotal;
}
