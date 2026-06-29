package com.kb.ai.dto;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * Java → Python {@code POST /ai/qa} 入参。
 * {@code allowedArticleIds=null} 表示不限（Chroma 只含在线知识，由上游保证已过滤）；
 * 空列表表示范围内无可命中知识 → Python 返回 no_hit。
 */
@Data
@AllArgsConstructor
@JsonInclude(JsonInclude.Include.NON_NULL)
public class AiQaRequest {

    private String question;

    @JsonProperty("knowledge_type")
    private String knowledgeType;

    @JsonProperty("top_k")
    private Integer topK;

    @JsonProperty("allowed_article_ids")
    private List<Long> allowedArticleIds;
}
