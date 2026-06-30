package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * Java → Python {@code POST /ai/index/remove} 入参。
 */
@Data
@AllArgsConstructor
@JsonInclude(JsonInclude.Include.NON_NULL)
public class AiIndexRemoveRequest {

    @JsonProperty("article_id")
    private Long articleId;

    @JsonProperty("knowledge_type")
    private String knowledgeType;
}
