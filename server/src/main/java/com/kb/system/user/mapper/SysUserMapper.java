package com.kb.system.user.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.kb.system.user.entity.SysUser;
import org.apache.ibatis.annotations.Param;
import org.apache.ibatis.annotations.Update;

import java.time.LocalDateTime;

public interface SysUserMapper extends BaseMapper<SysUser> {

    /** 仅更新最近登录时间/IP，不触碰其它字段与审计列。 */
    @Update("UPDATE sys_user SET last_login_time = #{time}, last_login_ip = #{ip} WHERE id = #{userId}")
    void updateLoginInfo(@Param("userId") Long userId,
                         @Param("time") LocalDateTime time,
                         @Param("ip") String ip);
}
