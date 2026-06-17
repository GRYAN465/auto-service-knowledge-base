"""M9.1 端到端验证（无需 Java、无需 LLM 令牌）。

造样例 .txt/.md → 解析 → 切分 → embed → 写 Chroma → 检索，
断言：dim==512、命中≥1、Top1 命中预期知识、范围过滤生效、向量库落盘。

用临时目录做向量库与样例存储，不污染真实 vector_store/。

运行（ai-service/ 目录下）::

    conda run -n kb-ai python scripts/validate_index.py
"""
import os
import shutil
import sys
import tempfile
from pathlib import Path

# 脚本在 ai-service/scripts/ 下，把 ai-service/ 加入 sys.path 以便 import app.*
_AI_SERVICE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(_AI_SERVICE))

# 临时目录隔离向量库与样例（必须在 import app.* 之前设置好环境变量）
_TMP = Path(tempfile.mkdtemp(prefix="kb-ai-validate-"))
os.environ["KB_AI_VECTOR_STORE_DIR"] = str(_TMP / "vector_store")
os.environ.setdefault("KB_AI_STORAGE_DIR", str(_TMP / "uploads"))

from app.core.config import settings  # noqa: E402
from app.core.logging import setup_logging  # noqa: E402
from app.services import chunker, embedding, parser  # noqa: E402
from app.services.vector_store import get_vector_store  # noqa: E402

SAMPLES = {
    "refund.txt": (
        "退款政策\n\n"
        "本店支持七天无理由退款。商品签收后七日内，在不影响二次销售的前提下，"
        "可申请退款。生鲜、定制类商品不支持无理由退款。\n\n"
        "退款将在审核通过后 1-3 个工作日原路退回到原支付账户。"
    ),
    "shipping.md": (
        "# 配送说明\n\n"
        "- 默认顺丰快递，全国大部分地区次日达。\n"
        "- 偏远地区预计 3-5 个工作日送达。\n\n"
        "## 运费规则\n\n"
        "订单满 99 元包邮，未满收取 10 元运费。"
    ),
}


def main() -> int:
    setup_logging()
    storage = Path(settings.storage_dir)
    storage.mkdir(parents=True, exist_ok=True)
    for name, content in SAMPLES.items():
        (storage / name).write_text(content, encoding="utf-8")

    store = get_vector_store()
    dim_seen = 0
    for article_id, name in [(90001, "refund.txt"), (90002, "shipping.md")]:
        text = parser.extract_text(name)  # 相对路径按 storage_dir 解析
        chunks = chunker.chunk_text(text)
        vectors, dim_seen = embedding.embed_texts(chunks)
        n = store.upsert(article_id, chunks, vectors)
        print(f"[index] article={article_id} {name}: chunks={len(chunks)} dim={dim_seen} upserted={n}")

    # 全量检索
    q = "退款政策是怎样的？多久到账？"
    qvec, qdim = embedding.embed_query(q)
    hits = store.query(qvec, top_k=3)
    print(f"\n[query] {q!r} → {len(hits)} 命中：")
    for h in hits:
        preview = h["text"][:40].replace("\n", " ")
        print(f"  article={h['article_id']} seq={h['seq']} score={h['score']} | {preview}…")

    # 范围过滤：只允许 90002，命中应全部来自 90002（验证 $in 过滤，供 M9.3 权限收口）
    scoped = store.query(qvec, top_k=3, allowed_article_ids=[90002])
    print(f"\n[query·allowed=90002] → {len(scoped)} 命中（应均来自 90002）")

    # 断言
    assert qdim == 512, f"期望查询向量 512 维，实得 {qdim}"
    assert dim_seen == 512, f"期望文档向量 512 维，实得 {dim_seen}"
    assert len(hits) >= 1, "检索零命中"
    assert hits[0]["article_id"] == 90001, f"Top1 应为退款政策(90001)，实得 {hits[0]['article_id']}"
    assert all(h["article_id"] == 90002 for h in scoped), "范围过滤未生效"
    persist = Path(settings.vector_store_dir)
    assert persist.exists(), f"向量库未落盘：{persist}"

    print("\n[OK] dim=512 · 命中≥1 · Top1=退款政策 · 范围过滤生效 · 向量库已落盘")
    print(f"     落盘目录（临时，验证后清理）：{persist}")

    # 清理：先释放 client 引用，再删临时目录（Windows 下 sqlite 句柄占用时忽略错误）
    store.remove_article(90001)
    store.remove_article(90002)
    del store
    shutil.rmtree(_TMP, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
