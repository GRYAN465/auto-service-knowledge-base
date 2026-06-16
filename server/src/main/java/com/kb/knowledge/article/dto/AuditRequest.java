package com.kb.knowledge.article.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

/**
 * 审核入参：auditStatus ∈ PASS/REJECT；REJECT 时 opinion 建议必填。
 */
@Data
public class AuditRequest {

    @NotBlank(message = "审核结论不能为空")
    private String auditStatus;

    private String opinion;
}
