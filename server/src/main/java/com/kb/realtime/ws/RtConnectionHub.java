package com.kb.realtime.ws;

import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;

import java.io.IOException;
import java.util.concurrent.ConcurrentHashMap;

/**
 * 实时辅助 WebSocket 连接中枢：sessionId → 在线连接。
 *
 * <p>由 {@link RtWebSocketHandler} 在连接建立/关闭时维护；RtAsrService / RtRecommendService
 * 通过 {@link #send} 把 transcript / recommendation / session_end 推给坐席面板。
 * 无在线连接时静默跳过推送（不报错，符合设计 §4.3 第 3 条）。
 *
 * <p>一个坐席一个会话一条连接；同 sessionId 重复连接时关闭旧连接、保留新连接。
 */
@Component
@Slf4j
public class RtConnectionHub {

    private final ConcurrentHashMap<Long, WebSocketSession> sessions = new ConcurrentHashMap<>();

    /** 注册连接；若该 sessionId 已有旧连接则先关闭旧连接。 */
    public void register(Long sessionId, WebSocketSession session) {
        WebSocketSession old = sessions.put(sessionId, session);
        if (old != null && old.isOpen() && !old.getId().equals(session.getId())) {
            try {
                old.close();
            } catch (IOException e) {
                log.debug("关闭旧 RT 连接失败 sessionId={}: {}", sessionId, e.toString());
            }
        }
        log.debug("RT 连接已注册 sessionId={}（在线={}）", sessionId, sessions.size());
    }

    /** 注销连接；仅当传入会话与已注册的一致才移除（避免被后注册的新连接误删）。 */
    public void unregister(Long sessionId, WebSocketSession session) {
        sessions.remove(sessionId, session);
        log.debug("RT 连接已注销 sessionId={}（在线={}）", sessionId, sessions.size());
    }

    public boolean hasConnection(Long sessionId) {
        WebSocketSession s = sessions.get(sessionId);
        return s != null && s.isOpen();
    }

    /** 推送文本消息；无在线连接返回 false（调用方静默跳过）。 */
    public boolean send(Long sessionId, String json) {
        WebSocketSession s = sessions.get(sessionId);
        if (s == null || !s.isOpen()) {
            return false;
        }
        try {
            s.sendMessage(new TextMessage(json));
            return true;
        } catch (IOException e) {
            log.warn("RT 推送失败 sessionId={}: {}", sessionId, e.toString());
            try {
                s.close();
            } catch (IOException ignored) {
            }
            sessions.remove(sessionId, s);
            return false;
        }
    }
}