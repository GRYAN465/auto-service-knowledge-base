"""LLM 客户端：ms-agent 框架驱动第三方 OpenAI 兼容接口（M9.2 智能问答用）。

经 ms-agent（魔搭 ModelScope）封装而非直接裸调 openai SDK——为对齐二期锁定栈，
也为后续模块 10「实时辅助」复用 ms-agent 的 agentic 能力（工具/MCP/多步）。本里
程碑只做单轮总结，底层仍打同一个 OpenAI 兼容 base_url。

未配 base_url/api_key/model 三件套时 is_configured() 为假，由 qa.py 走抽取式兜底；
即便已配，generate 端点抖动抛错也由 qa.py 捕获兜底——LLM 永不让问答 500。
"""
from __future__ import annotations

from app.core.config import persist_runtime_llm, settings
from app.core.logging import logger

_llm = None  # 懒加载单例（首次 chat 时构造）


def is_configured() -> bool:
    """base_url / api_key / model 三者齐备才视为已接 LLM。"""
    return bool(settings.llm_base_url and settings.llm_api_key and settings.llm_model)


def _build_llm(base_url: str, api_key: str, model: str, temperature: float, max_tokens: int):
    """按给定参数构造 ms-agent LLM（service=openai → 底层 openai.OpenAI(base_url, api_key)）。

    单例（_get_llm）与临时测试（test_config）共用同一构造逻辑，避免漂移。
    """
    from omegaconf import OmegaConf
    from ms_agent.llm import LLM

    cfg = OmegaConf.create(
        {
            "llm": {
                "service": "openai",
                "model": model,
                "openai_base_url": base_url,
                "openai_api_key": api_key,
            },
            # 按 chat.completions.create 语义；ms-agent 会按签名过滤透传
            "generation_config": {
                "temperature": temperature,
                "max_tokens": max_tokens,
                "stream": False,
            },
        }
    )
    return LLM.from_config(cfg)


def _get_llm():
    """构造并缓存单例 LLM（用当前 settings.llm_*）。配置变更后须先 reset()。"""
    global _llm
    if _llm is None:
        logger.info("初始化 LLM（ms-agent/openai）：model=%s base_url=%s",
                    settings.llm_model, settings.llm_base_url)
        _llm = _build_llm(settings.llm_base_url, settings.llm_api_key, settings.llm_model,
                          settings.llm_temperature, settings.llm_max_tokens)
    return _llm


def reset() -> None:
    """置空单例，使下次 chat 按最新 settings.llm_* 重建。配置下发后必调。"""
    global _llm
    _llm = None


def apply_config(cfg: dict) -> None:
    """运行时应用 LLM 配置（M9.4，Java 下发）：改 settings.llm_* + 落盘 + 重置单例。

    Java 为权威源并已解析好待应用值（含「留空保原值」）；本服务原样应用。三者留空即视为
    停用 LLM → is_configured() 转假 → 问答回退抽取式。
    """
    settings.llm_base_url = (cfg.get("base_url") or "").strip()
    settings.llm_api_key = (cfg.get("api_key") or "").strip()
    settings.llm_model = (cfg.get("model") or "").strip()
    if cfg.get("temperature") is not None:
        settings.llm_temperature = float(cfg["temperature"])
    if cfg.get("max_tokens") is not None:
        settings.llm_max_tokens = int(cfg["max_tokens"])
    persist_runtime_llm(
        {
            "base_url": settings.llm_base_url,
            "api_key": settings.llm_api_key,
            "model": settings.llm_model,
            "temperature": settings.llm_temperature,
            "max_tokens": settings.llm_max_tokens,
        }
    )
    reset()
    logger.info("LLM 配置已更新：configured=%s model=%s base_url=%s",
                is_configured(), settings.llm_model, settings.llm_base_url)


def test_config(cfg: dict) -> str:
    """用候选值建临时 LLM 做一次极短生成，返回文本（连通成功）或抛异常（失败）。

    不改 settings、不动单例、不落盘——纯连通性探测。三件套须齐备。
    """
    from ms_agent.llm import Message

    base_url = (cfg.get("base_url") or "").strip()
    api_key = (cfg.get("api_key") or "").strip()
    model = (cfg.get("model") or "").strip()
    if not (base_url and api_key and model):
        raise ValueError("base_url/api_key/model 三者必填")
    temperature = float(cfg.get("temperature", settings.llm_temperature))
    max_tokens = int(cfg.get("max_tokens", 64))

    llm = _build_llm(base_url, api_key, model, temperature, max_tokens)
    out = llm.generate([Message(role="user", content="回复两个字：你好")])
    return (getattr(out, "content", "") or "").strip()


def chat(system: str, user: str) -> str:
    """单轮对话：给定 system/user 提示 → 返回答案文本。

    非流式 generate 返回单个 Message，取 .content。端点不可用时 ms-agent 重试后
    抛异常，交由调用方（qa.py）捕获并兜底——本函数不吞异常。
    """
    from ms_agent.llm import Message

    messages = [
        Message(role="system", content=system),
        Message(role="user", content=user),
    ]
    out = _get_llm().generate(messages)
    return (getattr(out, "content", "") or "").strip()
