package com.kb.security;

import java.util.Set;

/**
 * 权限码解析器：按用户 id 取其全部权限码集合。
 *
 * <p>定义在 security 包，由业务层（{@code system.permission.PermissionService}）实现并注入，
 * 让 {@link JwtAuthenticationFilter} 不直接依赖具体 mapper（策略/依赖倒置）。
 */
public interface UserPermissionResolver {

    Set<String> resolvePermissions(Long userId);
}
