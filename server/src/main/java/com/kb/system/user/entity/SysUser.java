package com.kb.system.user.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

import java.time.LocalDateTime;

/**
 * 人员账号（表 sys_user）。{@code password} 为 BCrypt 密文。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("sys_user")
public class SysUser extends BaseEntity {

    private Long orgId;
    private String username;
    private String password;
    private String realName;
    private String nickname;
    private String avatar;
    private String email;
    private String phone;
    private String gender;
    /** ENABLED / DISABLED。 */
    private String status;
    private LocalDateTime expireTime;
    private LocalDateTime lastLoginTime;
    private String lastLoginIp;
    private String remark;
    /** 收藏隐私：0 公开，1 仅自己可见。 */
    private Integer favoritePrivate;
}
