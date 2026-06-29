package com.kb.openapi.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class OpenQaRequest {

    @NotBlank(message = "问题不能为空")
    private String question;

    private Integer topK = 5;

    /** 可选：限定知识类型（SCRIPT/TRAIN/PRODUCT/OFFICE）。 */
    private String knowledgeType;
}
