package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;

/**
 * Java → Python {@code POST /ai/llm/config} 入参（snake_case 对齐 FastAPI 契约，见 app/schemas/ai.py）。
 * 停用 LLM 时三者传空串 → Python is_configured() 转假 → 问答回退抽取式。
 */
@Data
@AllArgsConstructor
@JsonInclude(JsonInclude.Include.NON_NULL)
public class AiLlmConfigPush {

    @JsonProperty("base_url")
    private String baseUrl;

    @JsonProperty("api_key")
    private String apiKey;

    private String model;

    private Double temperature;

    @JsonProperty("max_tokens")
    private Integer maxTokens;
}
