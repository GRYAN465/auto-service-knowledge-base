package com.kb.ai.dto;



import com.fasterxml.jackson.annotation.JsonProperty;

import lombok.AllArgsConstructor;

import lombok.Data;

import lombok.NoArgsConstructor;



import java.util.List;



/** Java → Python {@code POST /ai/recommend/match} 入参。 */

@Data

@AllArgsConstructor

@NoArgsConstructor

public class AiRecommendMatchRequest {



    /** 兼容旧版；优先使用 {@link #profileSegments}。 */

    @JsonProperty("profile_text")

    private String profileText;



    /** 分层画像段 + 权值（即时搜索 > 近期搜索 > 点赞 > pin）。 */

    @JsonProperty("profile_segments")

    private List<AiProfileSegment> profileSegments;



    @JsonProperty("tag_items")

    private List<AiRecommendTagItem> tagItems;



    @JsonProperty("article_items")

    private List<AiRecommendArticleItem> articleItems;



    @JsonProperty("top_tags")

    private Integer topTags;



    @JsonProperty("top_articles")

    private Integer topArticles;



    @Data

    @AllArgsConstructor

    @NoArgsConstructor

    public static class AiProfileSegment {

        private String text;

        private Double weight;

    }



    @Data

    @AllArgsConstructor

    @NoArgsConstructor

    public static class AiRecommendTagItem {

        private Long id;

        private String name;

    }



    @Data

    @AllArgsConstructor

    @NoArgsConstructor

    public static class AiRecommendArticleItem {

        private Long id;

        private String text;

    }

}

