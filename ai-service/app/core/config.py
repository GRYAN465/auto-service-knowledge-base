"""服务配置（优先读环境变量，KB_AI_* 前缀）。

二期模块 9 智能问答的可调项集中在此：
- 文件存储 / 向量库落盘目录
- 本地 embedding 模型与切分参数
- LLM（OpenAI 兼容第三方接口，M9.2 用）

M9.4 起 LLM 配置支持运行时下发：Java 编排层把管理员在客户端填的配置 POST 到本服务，
本服务改 settings.llm_* 并落盘 ``runtime_llm_config.json``；模块加载时若该文件存在则
覆盖环境变量默认 → 本服务自身重启也不丢配置（环境变量只在启动时读一次）。
"""
import json
import os
from pathlib import Path

# ai-service/ 根目录（本文件位于 ai-service/app/core/config.py）
_BASE_DIR = Path(__file__).resolve().parents[2]
# 仓库根 auto-service-knowledge-base/（ai-service 的上一级）
_REPO_ROOT = _BASE_DIR.parent


class Settings:
    app_name: str = "kb-ai-service"
    env: str = os.getenv("KB_AI_ENV", "dev")

    # —— 文件存储 ——
    # 附件存储根目录；kb_attachment.file_path 存相对路径（yyyy/MM/uuid.ext），
    # 在此目录下解析。默认对齐 Java LocalFileStorage 的 ./uploads（部署时按需用
    # KB_AI_STORAGE_DIR 覆盖到 Java 实际写入的同一目录）。
    storage_dir: str = os.getenv("KB_AI_STORAGE_DIR", str(_REPO_ROOT / "uploads"))

    # —— 向量库（Chroma，进程内嵌、落盘）——
    vector_store_dir: str = os.getenv("KB_AI_VECTOR_STORE_DIR", str(_BASE_DIR / "vector_store"))
    collection_name: str = os.getenv("KB_AI_COLLECTION", "kb_knowledge")

    # —— Embedding（本地 bge-zh，CPU）——
    # 默认指向本地下载的模型目录；不存在时 embedding 层回退为按模型名联网拉取。
    embedding_model: str = os.getenv(
        "KB_AI_EMBEDDING_MODEL", str(_BASE_DIR / "models" / "bge-small-zh-v1.5")
    )
    embedding_device: str = os.getenv("KB_AI_EMBEDDING_DEVICE", "cpu")

    # —— 切分（按长度 + 重叠）——
    chunk_size: int = int(os.getenv("KB_AI_CHUNK_SIZE", "500"))
    chunk_overlap: int = int(os.getenv("KB_AI_CHUNK_OVERLAP", "80"))

    # —— LLM：第三方 OpenAI 兼容接口（M9.2 智能问答用，经 ms-agent 框架驱动）——
    # 三者齐备才视为已配置；任一缺失则 /ai/qa 退化为抽取式兜底（返回高分原文片段 + 引用）。
    llm_base_url: str = os.getenv("KB_AI_LLM_BASE_URL", "")
    llm_api_key: str = os.getenv("KB_AI_LLM_API_KEY", "")
    llm_model: str = os.getenv("KB_AI_LLM_MODEL", "")
    # 生成参数（按 OpenAI chat.completions 语义；ms-agent 会按签名过滤透传）
    llm_temperature: float = float(os.getenv("KB_AI_LLM_TEMPERATURE", "0.2"))
    llm_max_tokens: int = int(os.getenv("KB_AI_LLM_MAX_TOKENS", "1024"))

    # —— 问答检索 ——
    # 相关性下限（余弦相似度）：低于此分的命中视为不相关而丢弃，用于「无相关知识」短路。
    qa_min_score: float = float(os.getenv("KB_AI_QA_MIN_SCORE", "0.3"))

    # —— LLM 运行时配置落盘（M9.4）——
    # 管理员在客户端保存配置后写此文件；本服务重启时覆盖上面的 env 默认。进 .gitignore。
    runtime_llm_config_path: str = os.getenv(
        "KB_AI_RUNTIME_LLM_CONFIG", str(_BASE_DIR / "runtime_llm_config.json")
    )


settings = Settings()


# —— LLM 运行时配置：落盘字段 ↔ settings.llm_* 的映射（写盘/回读共用一份键定义）——
_RUNTIME_LLM_KEYS = ("base_url", "api_key", "model", "temperature", "max_tokens")


def persist_runtime_llm(cfg: dict) -> None:
    """把当前 LLM 配置写入 runtime_llm_config.json（仅 _RUNTIME_LLM_KEYS）。

    由 llm.apply_config 在应用配置后调用，使本服务自身重启不丢配置。写盘失败只告警不抛，
    避免影响在线请求（库里已是权威源，Java 启动会再补发）。
    """
    data = {k: cfg.get(k) for k in _RUNTIME_LLM_KEYS if cfg.get(k) is not None}
    try:
        Path(settings.runtime_llm_config_path).write_text(
            json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8"
        )
    except OSError as e:  # 仅告警，不影响在线请求
        from app.core.logging import logger

        logger.warning("写入 runtime_llm_config.json 失败：%s", e)


def _overlay_runtime_llm() -> None:
    """模块加载末尾：若落盘配置存在则覆盖 settings.llm_*（在 env 默认之后生效）。"""
    path = Path(settings.runtime_llm_config_path)
    if not path.exists():
        return
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return  # 文件损坏则忽略，沿用 env 默认
    if not isinstance(data, dict):
        return
    if data.get("base_url") is not None:
        settings.llm_base_url = str(data["base_url"])
    if data.get("api_key") is not None:
        settings.llm_api_key = str(data["api_key"])
    if data.get("model") is not None:
        settings.llm_model = str(data["model"])
    if data.get("temperature") is not None:
        settings.llm_temperature = float(data["temperature"])
    if data.get("max_tokens") is not None:
        settings.llm_max_tokens = int(data["max_tokens"])


_overlay_runtime_llm()
