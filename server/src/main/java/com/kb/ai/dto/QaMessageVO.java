package com.kb.ai.dto;

import lombok.Data;

import java.time.LocalDateTime;
import java.util.List;

/**
 * 会话历史中的一问一答（camelCase），含该答案的引用。
 */
@Data
public class QaMessageVO {

    private Long id;
    private Long sessionId;
    private String question;
    private String answer;
    private String mode;
    private Integer topK;
    private LocalDateTime createTime;
    private List<CitationVO> citations;
}
