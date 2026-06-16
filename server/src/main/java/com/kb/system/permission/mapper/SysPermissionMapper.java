package com.kb.system.permission.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.kb.system.permission.entity.SysPermission;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

import java.util.List;

public interface SysPermissionMapper extends BaseMapper<SysPermission> {

    /**
     * 用户被授予的全部权限节点（经 角色→角色权限 关联，去重，按 sort 升序）。
     * 含 DIR/MENU/BUTTON：MENU/DIR 装配菜单树，权限码用于接口与按钮鉴权。
     */
    @Select("SELECT DISTINCT p.id, p.parent_id, p.name, p.type, p.permission_code, p.route, p.icon, p.sort, p.status "
            + "FROM sys_permission p "
            + "JOIN sys_role_permission rp ON rp.permission_id = p.id "
            + "JOIN sys_user_role ur ON ur.role_id = rp.role_id "
            + "JOIN sys_role r ON r.id = ur.role_id "
            + "WHERE ur.user_id = #{userId} "
            + "AND p.status = 'ENABLED' AND p.deleted = 0 "
            + "AND r.status = 'ENABLED' AND r.deleted = 0 "
            + "ORDER BY p.parent_id, p.sort, p.id")
    List<SysPermission> selectGrantedByUserId(@Param("userId") Long userId);
}
