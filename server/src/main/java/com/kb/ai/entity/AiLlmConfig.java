package com.kb.ai.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * LLM 运行时接入配置（ai_llm_config）。单行配置（恒定 id=1），非软删；
 * 管理员在 Qt 客户端填写，由 Java 落库（权威源）并下发 Python（无需重启）。
 * api_key 本地明文存（同 yml 内 jwt secret 级别），对外只回掩码、绝不回传明文。
 */
@Data
@TableName("ai_llm_config")
public class AiLlmConfig {

    @TableId(type = IdType.AUTO)
    private Long id;

    /** OpenAI 兼容端点（通常以 /v1 结尾）。 */
    private String baseUrl;
    /** API 密钥（明文存储，对外掩码）。 */
    private String apiKey;
    private String model;
    private Double temperature;
    private Integer maxTokens;
    /** 1 启用 / 0 停用。 */
    private Integer enabled;
    private Long updateBy;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
