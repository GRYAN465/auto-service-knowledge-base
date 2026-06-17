package com.kb.ai.dto;

import lombok.Data;

import java.util.List;

/**
 * Python → Java {@code POST /ai/qa} 出参。字段名与契约一致，无需 @JsonProperty。
 */
@Data
public class AiQaResponse {

    private String answer;
    private List<AiCitation> citations;
    /** llm/extractive/no_hit */
    private String mode;
}
