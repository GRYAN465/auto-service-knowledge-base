package com.kb.system.auth.dto;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * {@code GET /auth/me} 响应：当前用户 + 角色 + 权限码 + 菜单树（驱动客户端导航与按钮）。
 * 由 {@code AuthService.me()} 从 DB(sys_user / sys_role / sys_permission) 装配。
 */
@Data
@AllArgsConstructor
public class MeResponse {

    private UserVO user;
    private List<String> roles;
    private List<String> permissions;
    private List<MenuVO> menus;
}
