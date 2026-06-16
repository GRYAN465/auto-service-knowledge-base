"""服务配置。二期接入向量库 / Embedding / LLM 时在此扩展（优先读环境变量）。"""
import os


class Settings:
    app_name: str = "kb-ai-service"
    env: str = os.getenv("KB_AI_ENV", "dev")

    # —— 以下为二期占位，接入时由环境变量注入 ——
    vector_store_url: str = os.getenv("KB_AI_VECTOR_STORE_URL", "")
    embedding_model: str = os.getenv("KB_AI_EMBEDDING_MODEL", "")
    llm_model: str = os.getenv("KB_AI_LLM_MODEL", "")


settings = Settings()
