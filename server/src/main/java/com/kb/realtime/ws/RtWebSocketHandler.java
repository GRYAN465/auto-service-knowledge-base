package com.kb.realtime.ws;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;

/**
 * 实时辅助 WebSocket 消息处理器：仅维护连接生命周期（转写/推荐由服务层经
 * {@link RtConnectionHub} 主动推送）。sessionId 从握手 URI 路径段 /ws/realtime/{sessionId} 解析。
 * 客户端→服务端消息 MVP 预留为 feedback，本期不处理。
 */
@Component
@RequiredArgsConstructor
@Slf4j
public class RtWebSocketHandler extends TextWebSocketHandler {

    private final RtConnectionHub hub;

    @Override
    public void afterConnectionEstablished(WebSocketSession session) {
        Long sessionId = resolveSessionId(session);
        if (sessionId == null) {
            try {
                session.close(CloseStatus.POLICY_VIOLATION.withReason("invalid sessionId"));
            } catch (Exception e) {
                log.debug("关闭非法 RT 连接失败 {}", e.toString());
            }
            return;
        }
        hub.register(sessionId, session);
    }

    @Override
    public void afterConnectionClosed(WebSocketSession session, CloseStatus status) {
        Long sessionId = resolveSessionId(session);
        if (sessionId != null) {
            hub.unregister(sessionId, session);
        }
    }

    /** MVP 预留：接收坐席 feedback，本期不处理。 */
    @Override
    protected void handleTextMessage(WebSocketSession session, TextMessage message) {
        // no-op（后续迭代：feedback adopt/ignore/pin）
    }

    private Long resolveSessionId(WebSocketSession session) {
        String path = session.getUri() == null ? "" : session.getUri().getPath();
        int idx = path.lastIndexOf('/');
        if (idx < 0 || idx == path.length() - 1) {
            return null;
        }
        try {
            return Long.valueOf(path.substring(idx + 1));
        } catch (NumberFormatException e) {
            return null;
        }
    }
}