package com.kb.recommend.dto;

import lombok.Data;

import java.util.List;

/** {@code GET /knowledge/recommend/home} 响应。 */
@Data
public class RecommendHomeVO {

    private List<RecommendArticleVO> items;
    private RecommendProfileSummaryVO profileSummary;
    /** true 表示行为不足或 AI 不可用，已降级为加权热门 */
    private boolean fallback;
}
