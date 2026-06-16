package com.kb.security;

import io.jsonwebtoken.Claims;
import jakarta.servlet.FilterChain;
import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.lang.NonNull;
import org.springframework.security.authentication.UsernamePasswordAuthenticationToken;
import org.springframework.security.core.authority.SimpleGrantedAuthority;
import org.springframework.security.core.context.SecurityContextHolder;
import org.springframework.web.filter.OncePerRequestFilter;

import java.io.IOException;
import java.util.List;
import java.util.Set;

/**
 * JWT 鉴权过滤器：从 Authorization: Bearer 头解析 token，校验通过后加载该用户的权限码，
 * 写入安全上下文（authorities），支撑 {@code @PreAuthorize} 的接口级鉴权。
 *
 * <p>当前每请求查库装配权限码；后续可加 Redis/本地缓存优化（TODO）。
 */
public class JwtAuthenticationFilter extends OncePerRequestFilter {

    private final JwtTokenProvider tokenProvider;
    private final UserPermissionResolver permissionResolver;

    public JwtAuthenticationFilter(JwtTokenProvider tokenProvider, UserPermissionResolver permissionResolver) {
        this.tokenProvider = tokenProvider;
        this.permissionResolver = permissionResolver;
    }

    @Override
    protected void doFilterInternal(@NonNull HttpServletRequest request,
                                    @NonNull HttpServletResponse response,
                                    @NonNull FilterChain filterChain) throws ServletException, IOException {
        String header = request.getHeader("Authorization");
        if (header != null && header.startsWith("Bearer ")) {
            String token = header.substring(7);
            try {
                Claims claims = tokenProvider.parse(token).getPayload();
                Long userId = Long.valueOf(claims.getSubject());
                String username = claims.get("username", String.class);
                Set<String> permissions = permissionResolver.resolvePermissions(userId);
                LoginUser loginUser = new LoginUser(userId, username, permissions);
                List<SimpleGrantedAuthority> authorities = permissions.stream()
                        .map(SimpleGrantedAuthority::new)
                        .toList();
                UsernamePasswordAuthenticationToken authentication =
                        new UsernamePasswordAuthenticationToken(loginUser, null, authorities);
                SecurityContextHolder.getContext().setAuthentication(authentication);
            } catch (Exception ignored) {
                // token 无效/过期/用户失效：保持未认证，后续由安全链拒绝
            }
        }
        filterChain.doFilter(request, response);
    }
}
