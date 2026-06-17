package com.kb.openapi.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class OpenAppRequest {

    @NotBlank(message = "应用名称不能为空")
    private String appName;

    private String appKey;
    private String appSecret;
    private String status = "ENABLED";
    private Integer rateLimit = 60;
    private String scope = "search,detail";
    private String remark;
}
