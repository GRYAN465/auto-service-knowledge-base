package com.kb.system.permission;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.security.UserPermissionResolver;
import com.kb.system.permission.entity.SysPermission;
import com.kb.system.permission.mapper.SysPermissionMapper;
import com.kb.system.role.mapper.SysRoleMapper;
import com.kb.system.user.entity.SysUser;
import com.kb.system.user.mapper.SysUserMapper;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;

import java.util.LinkedHashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * 权限装配：取用户被授予的权限节点、抽取权限码。实现 {@link UserPermissionResolver} 供安全过滤器使用。
 */
@Service
public class PermissionService implements UserPermissionResolver {

    private final SysPermissionMapper permissionMapper;
    private final SysUserMapper userMapper;
    private final SysRoleMapper roleMapper;

    public PermissionService(SysPermissionMapper permissionMapper, SysUserMapper userMapper,
                             SysRoleMapper roleMapper) {
        this.permissionMapper = permissionMapper;
        this.userMapper = userMapper;
        this.roleMapper = roleMapper;
    }

    /** 用户被授予的全部权限节点（DIR/MENU/BUTTON，已按 sort 排序）。ADMIN 返回全部启用权限。 */
    public List<SysPermission> listGranted(Long userId) {
        List<String> roles = roleMapper.selectRoleCodesByUserId(userId);
        if (roles.contains("ADMIN")) {
            return permissionMapper.selectList(Wrappers.<SysPermission>lambdaQuery()
                    .eq(SysPermission::getStatus, "ENABLED")
                    .orderByAsc(SysPermission::getParentId)
                    .orderByAsc(SysPermission::getSort)
                    .orderByAsc(SysPermission::getId));
        }
        return permissionMapper.selectGrantedByUserId(userId);
    }

    /** 从权限节点抽取非空权限码集合（用于 @PreAuthorize 与客户端按钮级控制）。 */
    public Set<String> extractCodes(List<SysPermission> granted) {
        return granted.stream()
                .map(SysPermission::getPermissionCode)
                .filter(Objects::nonNull)
                .filter(code -> !code.isBlank())
                .collect(Collectors.toCollection(LinkedHashSet::new));
    }

    @Override
    public Set<String> resolvePermissions(Long userId) {
        SysUser user = userMapper.selectById(userId);
        if (user == null || !"ENABLED".equals(user.getStatus())
                || (user.getExpireTime() != null && user.getExpireTime().isBefore(LocalDateTime.now()))) {
            throw new IllegalStateException("用户不存在或不可用");
        }
        return extractCodes(listGranted(userId));
    }
}
