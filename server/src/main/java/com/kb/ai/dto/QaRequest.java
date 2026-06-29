package com.kb.ai.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

/**
 * 前端 → Java 问答入参（camelCase，Jackson 默认）。
 */
@Data
public class QaRequest {

    @NotBlank(message = "问题不能为空")
    private String question;

    private Integer topK = 5;

    /** 可选：限定知识分类（含子分类）；不传则在全体在线知识中检索。 */
    private Long categoryId;

    /** 可选：续问已有会话；不传则新建会话。 */
    private Long sessionId;

    /** 可选：限定知识类型（SCRIPT/TRAIN/PRODUCT/OFFICE）；不传默认 SCRIPT。 */
    private String knowledgeType;
}
