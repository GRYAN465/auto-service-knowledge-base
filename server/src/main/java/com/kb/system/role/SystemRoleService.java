package com.kb.system.role;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.BusinessException;
import com.kb.common.PageResult;
import com.kb.common.ResultCode;
import com.kb.system.role.dto.RoleRequest;
import com.kb.system.role.dto.RoleVO;
import com.kb.system.role.entity.SysRole;
import com.kb.system.role.entity.SysRolePermission;
import com.kb.system.role.mapper.SysRoleMapper;
import com.kb.system.role.mapper.SysRolePermissionMapper;
import com.kb.system.user.mapper.SysUserRoleMapper;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.util.List;

@Service
public class SystemRoleService {

    private final SysRoleMapper roleMapper;
    private final SysRolePermissionMapper rolePermissionMapper;
    private final SysUserRoleMapper userRoleMapper;

    public SystemRoleService(SysRoleMapper roleMapper, SysRolePermissionMapper rolePermissionMapper,
                             SysUserRoleMapper userRoleMapper) {
        this.roleMapper = roleMapper;
        this.rolePermissionMapper = rolePermissionMapper;
        this.userRoleMapper = userRoleMapper;
    }

    public PageResult<RoleVO> page(long page, long pageSize, String keyword, String status) {
        IPage<SysRole> data = roleMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<SysRole>lambdaQuery()
                        .and(StringUtils.hasText(keyword), q -> q
                                .like(SysRole::getName, keyword)
                                .or()
                                .like(SysRole::getCode, keyword))
                        .eq(StringUtils.hasText(status), SysRole::getStatus, status)
                        .orderByAsc(SysRole::getSort)
                        .orderByAsc(SysRole::getId));
        List<RoleVO> records = data.getRecords().stream()
                .map(role -> RoleVO.from(role, rolePermissionMapper.selectPermissionIdsByRoleId(role.getId())))
                .toList();
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(), records);
    }

    public List<SysRole> options() {
        return roleMapper.selectList(Wrappers.<SysRole>lambdaQuery()
                .eq(SysRole::getStatus, "ENABLED")
                .orderByAsc(SysRole::getSort)
                .orderByAsc(SysRole::getId));
    }

    public RoleVO get(Long id) {
        SysRole role = roleMapper.selectById(id);
        if (role == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "角色不存在");
        }
        return RoleVO.from(role, rolePermissionMapper.selectPermissionIdsByRoleId(id));
    }

    public RoleVO create(RoleRequest request) {
        ensureCodeUnique(request.getCode(), null);
        SysRole role = new SysRole();
        apply(role, request);
        roleMapper.insert(role);
        return get(role.getId());
    }

    public RoleVO update(Long id, RoleRequest request) {
        SysRole role = roleMapper.selectById(id);
        if (role == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "角色不存在");
        }
        ensureCodeUnique(request.getCode(), id);
        apply(role, request);
        roleMapper.updateById(role);
        return get(id);
    }

    public void assignPermissions(Long roleId, List<Long> permissionIds) {
        get(roleId);
        rolePermissionMapper.delete(Wrappers.<SysRolePermission>lambdaQuery()
                .eq(SysRolePermission::getRoleId, roleId));
        if (permissionIds == null || permissionIds.isEmpty()) {
            return;
        }
        for (Long permissionId : permissionIds.stream().distinct().toList()) {
            SysRolePermission relation = new SysRolePermission();
            relation.setRoleId(roleId);
            relation.setPermissionId(permissionId);
            rolePermissionMapper.insert(relation);
        }
    }

    public void delete(Long id) {
        SysRole role = roleMapper.selectById(id);
        if (role == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "角色不存在");
        }
        if ("ADMIN".equals(role.getCode())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "内置管理员角色不能删除");
        }
        if (userRoleMapper.countByRoleId(id) > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "角色已分配用户，不能删除");
        }
        rolePermissionMapper.delete(Wrappers.<SysRolePermission>lambdaQuery()
                .eq(SysRolePermission::getRoleId, id));
        roleMapper.deleteById(id);
    }

    private void ensureCodeUnique(String code, Long excludeId) {
        Long count = roleMapper.selectCount(Wrappers.<SysRole>lambdaQuery()
                .eq(SysRole::getCode, code)
                .ne(excludeId != null, SysRole::getId, excludeId));
        if (count > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "角色编码已存在");
        }
    }

    private static void apply(SysRole role, RoleRequest request) {
        role.setName(request.getName());
        role.setCode(request.getCode());
        role.setDataScope(request.getDataScope() == null ? "SELF" : request.getDataScope());
        role.setSort(request.getSort() == null ? 0 : request.getSort());
        role.setStatus(request.getStatus() == null ? "ENABLED" : request.getStatus());
        role.setRemark(request.getRemark());
    }
}
