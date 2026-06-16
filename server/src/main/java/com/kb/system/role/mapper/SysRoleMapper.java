package com.kb.system.role.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.kb.system.role.entity.SysRole;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Select;

import java.util.List;

public interface SysRoleMapper extends BaseMapper<SysRole> {

    /** 用户拥有的角色编码（启用且未删除）。 */
    @Select("SELECT r.code FROM sys_role r "
            + "JOIN sys_user_role ur ON ur.role_id = r.id "
            + "WHERE ur.user_id = #{userId} AND r.status = 'ENABLED' AND r.deleted = 0 "
            + "ORDER BY r.sort")
    List<String> selectRoleCodesByUserId(@Param("userId") Long userId);
}
