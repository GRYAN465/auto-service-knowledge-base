package com.kb.ai;

import lombok.extern.slf4j.Slf4j;
import org.springframework.boot.context.event.ApplicationReadyEvent;
import org.springframework.context.event.EventListener;
import org.springframework.stereotype.Component;

/**
 * 启动补发 LLM 配置（M9.4）：应用就绪后把库里「启用且配齐」的配置 best-effort 下发一次给 Python，
 * 兜住「管理员保存配置时 Python 还没起 / 之后重启过」的场景。失败仅告警，不阻断启动。
 */
@Slf4j
@Component
public class LlmConfigSyncRunner {

    private final LlmConfigService llmConfigService;

    public LlmConfigSyncRunner(LlmConfigService llmConfigService) {
        this.llmConfigService = llmConfigService;
    }

    @EventListener(ApplicationReadyEvent.class)
    public void resyncOnStartup() {
        try {
            llmConfigService.pushOnStartup();
        } catch (Exception e) {   // 兜底：补发逻辑内部已 try/catch，这里再防御一层
            log.warn("启动补发 LLM 配置异常（忽略）：{}", e.toString());
        }
    }
}
