package com.kb.statistics.dto;

import lombok.Data;

/**
 * 热门知识行（{@code GET /statistics/hot-article}）：按累计浏览量倒序的 ONLINE 知识。
 */
@Data
public class HotArticleVO {

    private Long id;
    private String title;
    private Long categoryId;
    private String categoryName;
    private String knowledgeType;
    private String status;
    private Long viewCount;
    private Long likeCount;
    private Long favoriteCount;
    private Long commentCount;
}
