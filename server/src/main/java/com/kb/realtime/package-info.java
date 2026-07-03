/**
 * 客服实时辅助模块（模块 10，已实现）：Mock ASR 转写 + WebSocket 实时推送 + 定时 RAG 推荐。
 *
 * <p>数据流：Mock ASR → POST /realtime/asr/push（RtAsrService 写 rt_transcript 并推 WS transcript）
 * → RtRecommendService 每 5s 扫描 ACTIVE 会话，拼接新 customer 转写调 AiClient.qa() 取 Top3
 * → WebSocket recommendation 推送给 Qt AgentAssistPage。
 *
 * <p>会话复用 qa_session（title 前缀「实时辅助-」），活跃态在 rt_active_session，转写在 rt_transcript；
 * WebSocket 端点 /ws/realtime/{sessionId}?token=。权限码 {@code realtime:assist}（V15 复用预留菜单 502）。
 */
package com.kb.realtime;
