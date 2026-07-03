"""模块 10 客服实时辅助 — Mock ASR 模拟器。

模拟 CC 系统的 ASR 推送行为：登录坐席账号拿 JWT → 创建实时会话 → 按 interval 顺序
推送一段预设对话（customer/agent 交替）→ 全部推完结束会话。用于坐席辅助面板的端到端联调。

用法（ai-service/ 目录下）::

    conda activate kb-ai
    python scripts/mock_asr.py --base-url http://localhost:8080/api \\
        --username admin --password 123456 --interval 3 --dialogue 0

参数：
    --base-url    Java 后端地址（含 /api 前缀），默认 http://localhost:8080/api
    --username    坐席账号（需有 realtime:assist 权限），默认 admin
    --password    坐席密码，默认 123456
    --interval    每段对话推送间隔（秒），默认 3
    --dialogue    选择第几组对话（从 0 起），默认 0
    --caller      主叫号码，默认 13800001234

注：当前种子库仅 admin（持有 realtime:assist 权限）；真实坐席账号待后续人员管理模块落地后补充。
"""
import argparse
import json
import sys
import time
from pathlib import Path

import requests

DIALOGUE_FILE = Path(__file__).resolve().parent / "mock_asr_dialogues.json"


def login(base_url: str, username: str, password: str) -> str:
    resp = requests.post(f"{base_url}/auth/login",
                        json={"username": username, "password": password}, timeout=10)
    resp.raise_for_status()
    body = resp.json()
    token = body.get("data", {}).get("token")
    if not token:
        raise RuntimeError(f"登录失败：{body}")
    return token


def start_session(base_url: str, token: str, caller: str) -> int:
    resp = requests.post(f"{base_url}/realtime/session/start",
                        headers=_auth(token),
                        json={"callerNumber": caller}, timeout=10)
    resp.raise_for_status()
    sid = resp.json().get("data", {}).get("sessionId")
    if not sid:
        raise RuntimeError(f"开启会话失败：{resp.json()}")
    return sid


def push_asr(base_url: str, token: str, session_id: int, speaker: str, content: str) -> None:
    resp = requests.post(f"{base_url}/realtime/asr/push",
                        headers=_auth(token),
                        json={"sessionId": session_id, "speaker": speaker, "content": content},
                        timeout=10)
    if not resp.ok:
        print(f"  [推送失败] {speaker}: {content}  -> {resp.status_code} {resp.text}", flush=True)
    else:
        print(f"  [{speaker}] {content}", flush=True)


def end_session(base_url: str, token: str, session_id: int) -> None:
    requests.post(f"{base_url}/realtime/session/end",
                  headers=_auth(token),
                  json={"sessionId": session_id}, timeout=10)


def _auth(token: str) -> dict:
    return {"Authorization": f"Bearer {token}"}


def main() -> int:
    ap = argparse.ArgumentParser(description="Mock ASR 模拟器（模块 10）")
    ap.add_argument("--base-url", default="http://localhost:8080/api")
    ap.add_argument("--username", default="admin")
    ap.add_argument("--password", default="123456")
    ap.add_argument("--interval", type=float, default=3.0)
    ap.add_argument("--dialogue", type=int, default=0)
    ap.add_argument("--caller", default="13800001234")
    args = ap.parse_args()

    dialogues = json.loads(DIALOGUE_FILE.read_text(encoding="utf-8"))
    if not dialogues:
        print("对话库为空", file=sys.stderr)
        return 1
    if args.dialogue >= len(dialogues):
        print(f"--dialogue 超出范围（共 {len(dialogues)} 组）", file=sys.stderr)
        return 1
    lines = dialogues[args.dialogue]

    print(f"登录 {args.username} …", flush=True)
    token = login(args.base_url, args.username, args.password)
    print(f"开始会话（主叫 {args.caller}）…", flush=True)
    session_id = start_session(args.base_url, token, args.caller)
    print(f"会话 sessionId={session_id}，开始推送 {len(lines)} 段对话", flush=True)

    try:
        for line in lines:
            push_asr(args.base_url, token, session_id, line["speaker"], line["content"])
            time.sleep(args.interval)
        print("对话推送完毕，结束会话…", flush=True)
    finally:
        end_session(args.base_url, token, session_id)
        print("会话已结束", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())