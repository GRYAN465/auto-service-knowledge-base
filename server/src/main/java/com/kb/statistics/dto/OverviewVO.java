package com.kb.statistics.dto;

import lombok.Data;

import java.util.List;

/**
 * 仪表盘汇总（{@code GET /statistics/overview}）：全局只读统计。
 *
 * <ul>
 *   <li>知识规模：总量 + 各状态量（在线 / 待审核 / 草稿 / 已下线）</li>
 *   <li>互动总量：累计浏览 / 点赞 / 收藏 / 评论（取 kb_article 计数列之和）</li>
 *   <li>基础规模：分类数 / 标签数</li>
 *   <li>今日动态：今日浏览 / 今日检索 / 今日新增知识</li>
 *   <li>分布：知识状态分布 / 知识类型分布（驱动客户端的分布条/小表）</li>
 * </ul>
 */
@Data
public class OverviewVO {

    private Long articleTotal;
    private Long articleOnline;
    private Long articlePendingAudit;
    private Long articleDraft;
    private Long articleOffline;

    private Long viewTotal;
    private Long likeTotal;
    private Long favoriteTotal;
    private Long commentTotal;

    private Long categoryCount;
    private Long tagCount;

    private Long todayViews;
    private Long todaySearches;
    private Long todayNewArticles;

    private List<NameCountVO> statusDist;
    private List<NameCountVO> typeDist;
}
