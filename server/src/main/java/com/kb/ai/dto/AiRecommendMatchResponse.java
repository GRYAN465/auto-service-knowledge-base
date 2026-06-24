package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

import java.util.List;

/** Python {@code POST /ai/recommend/match} 出参。 */
@Data
public class AiRecommendMatchResponse {

    @JsonProperty("top_tags")
    private List<AiRecommendTagScore> topTags;

    @JsonProperty("top_articles")
    private List<AiRecommendArticleScore> topArticles;

    @Data
    public static class AiRecommendTagScore {
        private Long id;
        private String name;
        private Double score;
    }

    @Data
    public static class AiRecommendArticleScore {
        private Long id;
        private Double score;
    }
}
