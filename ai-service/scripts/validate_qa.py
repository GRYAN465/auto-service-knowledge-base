"""M9.2 端到端验证（默认无需 Java、无需 LLM 令牌）。

造样例 .txt/.md → 索引（解析→切分→embed→写 Chroma）→ 问答（检索 + LLM 总结 /
抽取式兜底），断言：
- 正常提问：mode∈{llm, extractive}、citations 非空且 Top1=退款篇、answer 非空、snippet 存在；
- 空范围（allowed_article_ids=[]）：mode==no_hit 且 citations 空。

未配 KB_AI_LLM_* 时走抽取式兜底（mode=extractive），零令牌消耗；配齐后自动走 LLM
（mode=llm）。用临时目录做向量库与样例存储，不污染真实 vector_store/。

运行（ai-service/ 目录下）::

    C:/Users/13577/.conda/envs/kb-ai/python.exe scripts/validate_qa.py
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
_TMP = Path(tempfile.mkdtemp(prefix="kb-ai-validate-qa-"))
os.environ["KB_AI_VECTOR_STORE_DIR"] = str(_TMP / "vector_store")
os.environ.setdefault("KB_AI_STORAGE_DIR", str(_TMP / "uploads"))

from app.core.config import settings  # noqa: E402
from app.core.logging import setup_logging  # noqa: E402
from app.services import chunker, embedding, llm, parser, qa  # noqa: E402
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
    for article_id, name in [(90001, "refund.txt"), (90002, "shipping.md")]:
        text = parser.extract_text(name)  # 相对路径按 storage_dir 解析
        chunks = chunker.chunk_text(text)
        vectors, _ = embedding.embed_texts(chunks)
        n = store.upsert(article_id, chunks, vectors)
        print(f"[index] article={article_id} {name}: chunks={len(chunks)} upserted={n}")

    print(f"\n[llm] is_configured={llm.is_configured()}（False 则走抽取式兜底，零令牌）")

    # 正常提问
    q = "退款多久到账？"
    res = qa.answer(q, top_k=3)
    print(f"\n[qa] {q!r} → mode={res['mode']} citations={len(res['citations'])}")
    print(f"     answer: {res['answer'][:80]}…")
    for c in res["citations"]:
        print(f"     cite article={c['article_id']} score={c['score']} title={c['title']!r}"
              f" snippet={c['snippet'][:30]!r}…")

    # 空范围：无任何可命中知识 → no_hit
    scoped = qa.answer(q, top_k=3, allowed_article_ids=[])
    print(f"\n[qa·allowed=[]] → mode={scoped['mode']} citations={len(scoped['citations'])}"
          f" answer={scoped['answer']!r}")

    # 断言
    assert res["mode"] in {"llm", "extractive"}, f"期望 llm/extractive，实得 {res['mode']}"
    assert res["answer"].strip(), "答案为空"
    assert res["citations"], "引用为空"
    assert res["citations"][0]["article_id"] == 90001, \
        f"Top1 应为退款政策(90001)，实得 {res['citations'][0]['article_id']}"
    assert res["citations"][0]["snippet"], "Top1 引用缺 snippet"
    assert scoped["mode"] == "no_hit", f"空范围应 no_hit，实得 {scoped['mode']}"
    assert scoped["citations"] == [], "空范围不应有引用"

    print(f"\n[OK] 问答 mode={res['mode']} · Top1=退款政策 · snippet 存在 · 空范围 no_hit")
    print(f"     落盘目录（临时，验证后清理）：{settings.vector_store_dir}")

    # 清理：先释放 client 引用，再删临时目录（Windows 下 sqlite 句柄占用时忽略错误）
    store.remove_article(90001)
    store.remove_article(90002)
    del store
    shutil.rmtree(_TMP, ignore_errors=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
