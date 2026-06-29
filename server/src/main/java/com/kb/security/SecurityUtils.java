package com.kb.security;

import org.springframework.security.core.Authentication;
import org.springframework.security.core.context.SecurityContextHolder;

/**
 * 安全上下文工具：取当前登录用户。
 */
public final class SecurityUtils {

    private SecurityUtils() {
    }

    public static LoginUser getLoginUserOrNull() {
        Authentication auth = SecurityContextHolder.getContext().getAuthentication();
        if (auth != null && auth.getPrincipal() instanceof LoginUser loginUser) {
            return loginUser;
        }
        return null;
    }

    public static Long getUserIdOrNull() {
        LoginUser loginUser = getLoginUserOrNull();
        return loginUser == null ? null : loginUser.getUserId();
    }

    public static boolean hasAuthority(String permissionCode) {
        LoginUser loginUser = getLoginUserOrNull();
        return loginUser != null
                && loginUser.getPermissions() != null
                && loginUser.getPermissions().contains(permissionCode);
    }
}
