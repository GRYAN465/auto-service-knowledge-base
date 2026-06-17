package com.kb.ai.dto;

import lombok.Data;

/**
 * LLM 配置回显（camelCase，前端用）。绝不含明文 api_key——只回 {@code configured} 与掩码。
 */
@Data
public class LlmConfigVO {

    private String baseUrl;
    private String model;
    private Double temperature;
    private Integer maxTokens;
    /** 是否启用。 */
    private Boolean enabled;
    /** base_url/api_key/model 三件套是否齐备（已可走 LLM）。 */
    private Boolean configured;
    /** 掩码后的密钥（如 {@code sk-****ab12}）；未配置则为空。 */
    private String apiKeyMasked;
    /** 下发到 Python 的告警（保存成功但下发失败时非空）；GET 时恒为空。 */
    private String pushWarning;
}
