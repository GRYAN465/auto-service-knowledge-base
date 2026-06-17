package com.kb.ai.dto;

import lombok.Data;

/**
 * 保存/更新 LLM 配置入参（camelCase，前端用）。
 * {@code apiKey} 留空表示「保持原值不变」，填了才覆盖（避免掩码回显被当成明文存回）。
 */
@Data
public class LlmConfigRequest {

    private String baseUrl;
    /** 留空＝不修改；非空＝覆盖。 */
    private String apiKey;
    private String model;
    private Double temperature;
    private Integer maxTokens;
    private Boolean enabled;
}
