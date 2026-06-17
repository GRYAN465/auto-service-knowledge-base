package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * Java → Python {@code POST /ai/index} 入参（snake_case 对齐 FastAPI 契约，见 app/schemas/ai.py）。
 * 正文方案下 {@code texts} 通常单元素：标题 + 摘要 + 正文（去 HTML）。
 */
@Data
@AllArgsConstructor
@JsonInclude(JsonInclude.Include.NON_NULL)
public class AiIndexRequest {

    @JsonProperty("article_id")
    private Long articleId;

    private List<String> texts;
}
