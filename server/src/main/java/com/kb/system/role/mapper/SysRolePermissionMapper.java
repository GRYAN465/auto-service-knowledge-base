package com.kb.system.role.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.kb.system.role.entity.SysRolePermission;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

import java.util.List;

public interface SysRolePermissionMapper extends BaseMapper<SysRolePermission> {

    @Select("SELECT permission_id FROM sys_role_permission WHERE role_id = #{roleId} ORDER BY permission_id")
    List<Long> selectPermissionIdsByRoleId(@Param("roleId") Long roleId);

    @Select("SELECT COUNT(1) FROM sys_role_permission WHERE permission_id = #{permissionId}")
    long countByPermissionId(@Param("permissionId") Long permissionId);
}
