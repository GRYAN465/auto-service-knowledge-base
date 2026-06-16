package com.kb.security;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.Set;

/**
 * 安全上下文中的登录主体：当前用户 id、用户名、权限码集合。
 */
@Data
@AllArgsConstructor
public class LoginUser {

    private Long userId;
    private String username;
    private Set<String> permissions;
}
