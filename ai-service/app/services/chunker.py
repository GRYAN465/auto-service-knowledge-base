"""文本切分：段落感知的滑窗 + 重叠，输出适合 embedding 的文本块。

中文以字符计长（size/overlap 单位为字符）。bge-small-zh 上限约 512 token，
默认 size=500 字 + overlap=80 字，单块经 embedding 截断也不丢主要语义。
"""
from __future__ import annotations

import re

from app.core.config import settings

# 连续空白行视为段落分隔
_PARA_SPLIT = re.compile(r"\n\s*\n")
_TRAILING_WS = re.compile(r"[ \t]+\n")
_MANY_NL = re.compile(r"\n{3,}")


def _normalize(text: str) -> str:
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    text = _TRAILING_WS.sub("\n", text)
    text = _MANY_NL.sub("\n\n", text)
    return text.strip()


def _window(s: str, size: int, overlap: int) -> list[str]:
    """对超长段落做定长滑窗（带重叠）。"""
    step = max(1, size - overlap)
    out: list[str] = []
    i, n = 0, len(s)
    while i < n:
        out.append(s[i : i + size])
        if i + size >= n:
            break
        i += step
    return out


def chunk_text(
    text: str,
    size: int | None = None,
    overlap: int | None = None,
) -> list[str]:
    """段落优先打包成 <= size 的块，块间保留 overlap 字符上下文。返回非空块列表。"""
    size = size or settings.chunk_size
    overlap = overlap or settings.chunk_overlap
    if overlap >= size:
        overlap = size // 5  # 防御非法配置

    text = _normalize(text)
    if not text:
        return []

    # 段落，超长段落先滑窗拆成 <= size 的片段
    segments: list[str] = []
    for para in _PARA_SPLIT.split(text):
        para = para.strip()
        if not para:
            continue
        if len(para) <= size:
            segments.append(para)
        else:
            segments.extend(_window(para, size, overlap))

    # 贪心打包：累积段落直到接近 size；翻页时用上一块尾部 overlap 字符做衔接
    chunks: list[str] = []
    buf = ""
    for seg in segments:
        if not buf:
            buf = seg
        elif len(buf) + 1 + len(seg) <= size:
            buf = f"{buf}\n{seg}"
        else:
            chunks.append(buf)
            tail = buf[-overlap:] if overlap > 0 else ""
            # 仅当衔接后仍不超长才带上重叠，保证每块 <= size
            if tail and len(tail) + 1 + len(seg) <= size:
                buf = f"{tail}\n{seg}"
            else:
                buf = seg
    if buf:
        chunks.append(buf)

    return chunks
