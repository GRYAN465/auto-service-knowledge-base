package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

/**
 * Python → Java {@code POST /ai/index} 出参。
 */
@Data
public class AiIndexResponse {

    @JsonProperty("article_id")
    private Long articleId;

    @JsonProperty("chunk_count")
    private Integer chunkCount;

    private Integer dim;
}
