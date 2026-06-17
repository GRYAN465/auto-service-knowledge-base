package com.kb.ai;

import com.kb.ai.dto.LlmConfigRequest;
import com.kb.ai.dto.LlmConfigVO;
import com.kb.ai.dto.LlmTestVO;
import com.kb.common.Result;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

/**
 * LLM 运行时配置（模块 9，权限码 {@code ai:config}，仅管理员）：在 Qt 客户端填写/测试/保存
 * OpenAI 兼容接入参数，由 Java 落库并实时下发 Python（无需重启）。
 *
 * <p>外部路径 {@code /api/ai/llm-config}（连字符）区别于 Python 内部 {@code /ai/llm/config}。
 * 仅授 ADMIN（V11 只给 role 1 授 504）→ 普通用户访问返回 403。
 */
@Tag(name = "AI 配置")
@RestController
@RequestMapping("/ai/llm-config")
public class AiConfigController {

    private final LlmConfigService llmConfigService;

    public AiConfigController(LlmConfigService llmConfigService) {
        this.llmConfigService = llmConfigService;
    }

    @Operation(summary = "查看 LLM 配置（掩码，不回明文 key）")
    @GetMapping
    @PreAuthorize("hasAuthority('ai:config')")
    public Result<LlmConfigVO> get() {
        return Result.ok(llmConfigService.view());
    }

    @Operation(summary = "保存 LLM 配置（key 留空保持原值）并实时下发")
    @PutMapping
    @PreAuthorize("hasAuthority('ai:config')")
    public Result<LlmConfigVO> update(@RequestBody LlmConfigRequest request) {
        return Result.ok(llmConfigService.update(request));
    }

    @Operation(summary = "测试 LLM 连通性（不落库不改运行配置）")
    @PostMapping("/test")
    @PreAuthorize("hasAuthority('ai:config')")
    public Result<LlmTestVO> test(@RequestBody LlmConfigRequest request) {
        return Result.ok(llmConfigService.test(request));
    }
}
