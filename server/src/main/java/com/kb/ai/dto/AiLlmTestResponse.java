package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

/**
 * Python → Java {@code POST /ai/llm/test} 出参（snake_case）。成功/失败都 HTTP 200，由 {@code ok} 区分。
 */
@Data
public class AiLlmTestResponse {

    private Boolean ok;

    private String message;

    @JsonProperty("latency_ms")
    private Integer latencyMs;
}
