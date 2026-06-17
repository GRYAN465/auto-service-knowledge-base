package com.kb.ai.dto;

import lombok.Data;

import java.util.List;

/**
 * 问答结果（camelCase）。对外开放接口复用同一结构，此时 {@code sessionId}/{@code messageId} 为空（不落会话）。
 */
@Data
public class QaVO {

    private Long sessionId;
    private Long messageId;
    private String answer;
    /** llm/extractive/no_hit */
    private String mode;
    private List<CitationVO> citations;
}
