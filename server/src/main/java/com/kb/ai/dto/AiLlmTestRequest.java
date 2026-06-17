package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * Java → Python {@code POST /ai/llm/test} 入参（snake_case）。用候选值做一次极短生成，不落库不改单例。
 */
@Data
@AllArgsConstructor
@JsonInclude(JsonInclude.Include.NON_NULL)
public class AiLlmTestRequest {

    @JsonProperty("base_url")
    private String baseUrl;

    @JsonProperty("api_key")
    private String apiKey;

    private String model;

    private Double temperature;

    @JsonProperty("max_tokens")
    private Integer maxTokens;
}
