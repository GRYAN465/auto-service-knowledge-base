package com.kb.system.permission;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.system.permission.dto.PermissionRequest;
import com.kb.system.permission.dto.PermissionTreeVO;
import com.kb.system.permission.entity.SysPermission;
import com.kb.system.permission.mapper.SysPermissionMapper;
import com.kb.system.role.mapper.SysRolePermissionMapper;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

@Service
public class SystemPermissionAdminService {

    private final SysPermissionMapper permissionMapper;
    private final SysRolePermissionMapper rolePermissionMapper;

    public SystemPermissionAdminService(SysPermissionMapper permissionMapper,
                                        SysRolePermissionMapper rolePermissionMapper) {
        this.permissionMapper = permissionMapper;
        this.rolePermissionMapper = rolePermissionMapper;
    }

    public List<PermissionTreeVO> tree() {
        List<SysPermission> permissions = permissionMapper.selectList(Wrappers.<SysPermission>lambdaQuery()
                .orderByAsc(SysPermission::getSort)
                .orderByAsc(SysPermission::getId));
        return buildTree(permissions);
    }

    public SysPermission get(Long id) {
        SysPermission permission = permissionMapper.selectById(id);
        if (permission == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "权限不存在");
        }
        return permission;
    }

    public SysPermission create(PermissionRequest request) {
        SysPermission permission = new SysPermission();
        apply(permission, request);
        permissionMapper.insert(permission);
        return permission;
    }

    public SysPermission update(Long id, PermissionRequest request) {
        SysPermission permission = get(id);
        if (id.equals(request.getParentId())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "上级权限不能是自身");
        }
        apply(permission, request);
        permissionMapper.updateById(permission);
        return get(id);
    }

    public void delete(Long id) {
        get(id);
        Long childCount = permissionMapper.selectCount(Wrappers.<SysPermission>lambdaQuery()
                .eq(SysPermission::getParentId, id));
        if (childCount > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "存在下级权限，不能删除");
        }
        if (rolePermissionMapper.countByPermissionId(id) > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "权限已分配角色，不能删除");
        }
        permissionMapper.deleteById(id);
    }

    private static void apply(SysPermission permission, PermissionRequest request) {
        permission.setParentId(request.getParentId() == null ? 0L : request.getParentId());
        permission.setName(request.getName());
        permission.setType(request.getType());
        permission.setPermissionCode(request.getPermissionCode());
        permission.setRoute(request.getRoute());
        permission.setIcon(request.getIcon());
        permission.setSort(request.getSort() == null ? 0 : request.getSort());
        permission.setStatus(request.getStatus() == null ? "ENABLED" : request.getStatus());
    }

    private static List<PermissionTreeVO> buildTree(List<SysPermission> permissions) {
        Map<Long, PermissionTreeVO> nodes = new LinkedHashMap<>();
        Map<Long, Long> parentOf = new LinkedHashMap<>();
        permissions.stream()
                .sorted(Comparator.comparing(SysPermission::getSort, Comparator.nullsLast(Integer::compareTo))
                        .thenComparing(SysPermission::getId))
                .forEach(permission -> {
                    nodes.put(permission.getId(), new PermissionTreeVO(permission.getId(), permission.getParentId(),
                            permission.getName(), permission.getType(), permission.getPermissionCode(),
                            permission.getRoute(), permission.getIcon(), permission.getSort(),
                            permission.getStatus(), new ArrayList<>()));
                    parentOf.put(permission.getId(), permission.getParentId());
                });
        List<PermissionTreeVO> roots = new ArrayList<>();
        for (Map.Entry<Long, PermissionTreeVO> e : nodes.entrySet()) {
            PermissionTreeVO parent = nodes.get(parentOf.get(e.getKey()));
            if (parent == null) {
                roots.add(e.getValue());
            } else {
                parent.getChildren().add(e.getValue());
            }
        }
        return roots;
    }
}
