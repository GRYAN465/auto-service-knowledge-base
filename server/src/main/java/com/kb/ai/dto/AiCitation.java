package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.Data;

/**
 * Python → Java 问答引用项。{@code title} 在 Python 侧为占位「知识 #{id}」，由 Java 用 kb_article 回填真实标题。
 */
@Data
public class AiCitation {

    @JsonProperty("article_id")
    private Long articleId;

    private String title;
    private Double score;
    private String snippet;
}
