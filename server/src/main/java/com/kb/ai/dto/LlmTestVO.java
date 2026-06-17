package com.kb.ai.dto;

import lombok.Data;

/**
 * LLM 连通性测试结果（camelCase，前端用）。
 */
@Data
public class LlmTestVO {

    /** 是否连通并成功生成。 */
    private Boolean ok;
    /** 成功为模型返回文本片段，失败为错误原因。 */
    private String message;
    /** 一次极短生成的往返耗时（毫秒）。 */
    private Integer latencyMs;
}
