package com.kb.ai;

import com.kb.ai.dto.AiLlmTestResponse;
import com.kb.ai.dto.LlmConfigRequest;
import com.kb.ai.dto.LlmConfigVO;
import com.kb.ai.dto.LlmTestVO;
import com.kb.ai.entity.AiLlmConfig;
import com.kb.ai.mapper.AiLlmConfigMapper;
import com.kb.security.SecurityUtils;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.time.LocalDateTime;

/**
 * LLM 运行时配置编排（M9.4）：DB(单行 id=1) 为权威源，保存后 best-effort 下发 Python（无需重启）。
 *
 * <p>密钥处理：DB 明文存（本地项目，同 yml 内 jwt secret 级别）；对外 {@link #view()} 只回掩码 +
 * {@code configured} 布尔，绝不回传明文；{@link #update} 时 apiKey 留空＝保持原值，填了才覆盖。
 *
 * <p>下发语义：启用且三件套齐备才推真实值；停用/未配齐则推空串，令 Python {@code is_configured()} 转假
 * → 问答回退抽取式。Python 不可用时只存库 + 记告警，{@link LlmConfigSyncRunner} 在下次 Java 启动补发。
 */
@Slf4j
@Service
public class LlmConfigService {

    /** 单行配置固定 id（V11 已种空行）。 */
    private static final long ROW_ID = 1L;
    private static final double DEFAULT_TEMPERATURE = 0.2;
    private static final int DEFAULT_MAX_TOKENS = 1024;
    /** 连通性测试用较小的生成上限。 */
    private static final int TEST_MAX_TOKENS = 64;

    private final AiLlmConfigMapper configMapper;
    private final AiClient aiClient;

    public LlmConfigService(AiLlmConfigMapper configMapper, AiClient aiClient) {
        this.configMapper = configMapper;
        this.aiClient = aiClient;
    }

    /** 读 id=1；缺失（异常情况）则补建空行，保证后续 update 恒有行可改。 */
    public AiLlmConfig get() {
        AiLlmConfig cfg = configMapper.selectById(ROW_ID);
        if (cfg == null) {
            cfg = new AiLlmConfig();
            cfg.setId(ROW_ID);
            cfg.setTemperature(DEFAULT_TEMPERATURE);
            cfg.setMaxTokens(DEFAULT_MAX_TOKENS);
            cfg.setEnabled(0);
            cfg.setCreateTime(LocalDateTime.now());
            cfg.setUpdateTime(LocalDateTime.now());
            configMapper.insert(cfg);
        }
        return cfg;
    }

    /** 掩码视图（前端回填用，不含明文 key）。 */
    public LlmConfigVO view() {
        return toVO(get());
    }

    /** 保存：apiKey 留空保原值 → 存库 → best-effort 下发；返回最新视图（含下发告警）。 */
    public LlmConfigVO update(LlmConfigRequest req) {
        AiLlmConfig cfg = get();
        cfg.setBaseUrl(trimToNull(req.getBaseUrl()));
        cfg.setModel(trimToNull(req.getModel()));
        if (req.getTemperature() != null) {
            cfg.setTemperature(req.getTemperature());
        }
        if (req.getMaxTokens() != null) {
            cfg.setMaxTokens(req.getMaxTokens());
        }
        cfg.setEnabled(Boolean.TRUE.equals(req.getEnabled()) ? 1 : 0);
        if (StringUtils.hasText(req.getApiKey())) {   // 留空＝不改
            cfg.setApiKey(req.getApiKey().trim());
        }
        cfg.setUpdateBy(SecurityUtils.getUserIdOrNull());
        cfg.setUpdateTime(LocalDateTime.now());
        configMapper.updateById(cfg);

        String warning = pushBestEffort(cfg);
        LlmConfigVO vo = toVO(cfg);
        vo.setPushWarning(warning);
        return vo;
    }

    /** 测试连通：候选值（apiKey 留空用库里已存）转 Python 做极短生成；任何失败都转为 ok=false。 */
    public LlmTestVO test(LlmConfigRequest req) {
        AiLlmConfig cfg = get();
        String baseUrl = StringUtils.hasText(req.getBaseUrl()) ? req.getBaseUrl().trim() : cfg.getBaseUrl();
        String model = StringUtils.hasText(req.getModel()) ? req.getModel().trim() : cfg.getModel();
        String apiKey = StringUtils.hasText(req.getApiKey()) ? req.getApiKey().trim() : cfg.getApiKey();
        Double temperature = req.getTemperature() != null ? req.getTemperature() : cfg.getTemperature();

        LlmTestVO vo = new LlmTestVO();
        if (!StringUtils.hasText(baseUrl) || !StringUtils.hasText(model) || !StringUtils.hasText(apiKey)) {
            vo.setOk(false);
            vo.setMessage("base_url / api_key / model 三者必填");
            return vo;
        }
        try {
            AiLlmTestResponse resp = aiClient.testLlmConfig(baseUrl, apiKey, model, temperature, TEST_MAX_TOKENS);
            if (resp == null) {
                vo.setOk(false);
                vo.setMessage("AI 服务无响应");
            } else {
                vo.setOk(Boolean.TRUE.equals(resp.getOk()));
                vo.setMessage(resp.getMessage());
                vo.setLatencyMs(resp.getLatencyMs());
            }
        } catch (Exception e) {
            log.warn("调用 AI 服务测试 LLM 失败：{}", e.toString());
            vo.setOk(false);
            vo.setMessage("无法连接 AI 服务：" + e.getMessage());
        }
        return vo;
    }

    /** 启动补发：库里启用且配齐才下发（兜「存库时 Python 还没起」）。失败仅告警。 */
    public void pushOnStartup() {
        AiLlmConfig cfg = get();
        if (!isActive(cfg)) {
            log.info("LLM 未启用或未配齐，启动不下发");
            return;
        }
        try {
            aiClient.pushLlmConfig(cfg.getBaseUrl(), cfg.getApiKey(), cfg.getModel(),
                    cfg.getTemperature(), cfg.getMaxTokens());
            log.info("启动补发 LLM 配置成功：model={} base_url={}", cfg.getModel(), cfg.getBaseUrl());
        } catch (Exception e) {
            log.warn("启动补发 LLM 配置失败（AI 服务可能未就绪）：{}", e.toString());
        }
    }

    // ---------------------------------------------------------------- 私有

    /** best-effort 下发：启用且配齐推真实值，否则推空串令 Python 回退抽取式。失败返回告警文案。 */
    private String pushBestEffort(AiLlmConfig cfg) {
        try {
            if (isActive(cfg)) {
                aiClient.pushLlmConfig(cfg.getBaseUrl(), cfg.getApiKey(), cfg.getModel(),
                        cfg.getTemperature(), cfg.getMaxTokens());
            } else {
                aiClient.pushLlmConfig("", "", "", cfg.getTemperature(), cfg.getMaxTokens());
            }
            return null;
        } catch (Exception e) {
            log.warn("下发 LLM 配置到 Python 失败（已存库，下次启动补发）：{}", e.toString());
            return "配置已保存，但下发到 AI 服务失败（AI 服务可能未启动），重启 AI 服务后自动生效：" + e.getMessage();
        }
    }

    private LlmConfigVO toVO(AiLlmConfig cfg) {
        LlmConfigVO vo = new LlmConfigVO();
        vo.setBaseUrl(cfg.getBaseUrl());
        vo.setModel(cfg.getModel());
        vo.setTemperature(cfg.getTemperature() != null ? cfg.getTemperature() : DEFAULT_TEMPERATURE);
        vo.setMaxTokens(cfg.getMaxTokens() != null ? cfg.getMaxTokens() : DEFAULT_MAX_TOKENS);
        vo.setEnabled(toBool(cfg.getEnabled()));
        vo.setConfigured(isConfigured(cfg));
        vo.setApiKeyMasked(mask(cfg.getApiKey()));
        return vo;
    }

    /** 三件套齐备（与 Python is_configured 同义）。 */
    private static boolean isConfigured(AiLlmConfig cfg) {
        return StringUtils.hasText(cfg.getBaseUrl())
                && StringUtils.hasText(cfg.getApiKey())
                && StringUtils.hasText(cfg.getModel());
    }

    /** 启用且配齐 → 实际可走 LLM。 */
    private static boolean isActive(AiLlmConfig cfg) {
        return isConfigured(cfg) && toBool(cfg.getEnabled());
    }

    private static boolean toBool(Integer v) {
        return v != null && v == 1;
    }

    private static String mask(String key) {
        if (!StringUtils.hasText(key)) {
            return null;
        }
        String k = key.trim();
        if (k.length() <= 8) {
            return "****";
        }
        return k.substring(0, 4) + "****" + k.substring(k.length() - 4);
    }

    private static String trimToNull(String s) {
        if (s == null) {
            return null;
        }
        String t = s.trim();
        return t.isEmpty() ? null : t;
    }
}
