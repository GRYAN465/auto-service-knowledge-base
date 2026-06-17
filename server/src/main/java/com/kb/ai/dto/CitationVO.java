package com.kb.ai.dto;

import lombok.Data;

/**
 * 问答引用项（camelCase）。标题已由 Java 用 kb_article 回填为真实标题后返回前端。
 */
@Data
public class CitationVO {

    private Long articleId;
    private String title;
    private Double score;
    private String snippet;
}
