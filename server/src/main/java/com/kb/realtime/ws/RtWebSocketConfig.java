package com.kb.realtime.ws;

import lombok.RequiredArgsConstructor;
import org.springframework.context.annotation.Configuration;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;

/**
 * 实时辅助 WebSocket 端点注册：{@code /ws/realtime/{sessionId}}。
 * 配合 context-path /api，实际可连地址为 {@code ws://host/api/ws/realtime/{sessionId}?token={jwt}}。
 *
 * <p>WebSocket 路径不经过 SecurityConfig 的 JWT 头过滤（握手用 URL token），
 * 由 {@link RtHandshakeInterceptor} 自行校验；SecurityConfig 已放行该前缀。允许所有来源。
 */
@Configuration
@RequiredArgsConstructor
public class RtWebSocketConfig implements WebSocketConfigurer {

    private final RtWebSocketHandler handler;
    private final RtHandshakeInterceptor handshakeInterceptor;

    @Override
    public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
        // 用 ant 通配 /ws/realtime/* 匹配任意尾段作为 sessionId；
        // Spring WebSocket 注册不消费 path 变量，由 handler 从完整 URI 自行解析 sessionId。
        registry.addHandler(handler, "/ws/realtime/*")
                .addInterceptors(handshakeInterceptor)
                .setAllowedOriginPatterns("*");
    }
}