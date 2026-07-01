package com.kb.realtime.ws;

import io.jsonwebtoken.Claims;
import lombok.extern.slf4j.Slf4j;
import org.springframework.http.server.ServerHttpRequest;
import org.springframework.http.server.ServerHttpResponse;
import org.springframework.http.server.ServletServerHttpRequest;
import org.springframework.stereotype.Component;
import org.springframework.web.socket.WebSocketHandler;
import org.springframework.web.socket.server.HandshakeInterceptor;

import com.kb.security.JwtTokenProvider;

import java.net.URI;
import java.util.HashMap;
import java.util.Map;

/**
 * WebSocket 握手拦截器：从 URL 查询参数 {@code ?token=xxx} 解析 JWT（握手阶段无法用 Authorization 头），
 * 校验通过把 userId/username 塞进 attributes；失败拒绝握手（返回 false）。
 *
 * <p>token 走 URL 参数是设计文档原案；握手阶段才校验、不进业务日志，避免 token 明文入访问日志的长期暴露。
 */
@Component
@Slf4j
public class RtHandshakeInterceptor implements HandshakeInterceptor {

    private final JwtTokenProvider tokenProvider;

    public RtHandshakeInterceptor(JwtTokenProvider tokenProvider) {
        this.tokenProvider = tokenProvider;
    }

    @Override
    public boolean beforeHandshake(ServerHttpRequest request, ServerHttpResponse response,
                                   WebSocketHandler wsHandler, Map<String, Object> attributes) {
        String token = extractToken(request.getURI());
        if (token == null) {
            log.warn("RT WS 握手拒绝：缺少 token");
            return false;
        }
        try {
            Claims claims = tokenProvider.parse(token).getPayload();
            Long userId = Long.valueOf(claims.getSubject());
            String username = claims.get("username", String.class);
            attributes.put("userId", userId);
            attributes.put("username", username);
            return true;
        } catch (Exception e) {
            log.warn("RT WS 握手拒绝：token 无效 {}", e.toString());
            return false;
        }
    }

    @Override
    public void afterHandshake(ServerHttpRequest request, ServerHttpResponse response,
                               WebSocketHandler wsHandler, Exception exception) {
        // no-op
    }

    private String extractToken(URI uri) {
        String query = uri.getQuery();
        if (query == null) {
            return null;
        }
        for (String pair : query.split("&")) {
            int eq = pair.indexOf('=');
            if (eq > 0 && "token".equals(pair.substring(0, eq))) {
                return pair.substring(eq + 1);
            }
        }
        return null;
    }

    /** 供重组 ServerHttpRequest 时复用（未用上，预留）。 */
    @SuppressWarnings("unused")
    private static Map<String, Object> emptyAttrs() {
        return new HashMap<>();
    }

    @SuppressWarnings("unused")
    private static ServletServerHttpRequest asServlet(ServerHttpRequest req) {
        return req instanceof ServletServerHttpRequest s ? s : null;
    }
}