package com.kb.system.user.dto;

import com.kb.system.user.entity.SysUser;
import lombok.Data;

import java.time.LocalDateTime;
import java.util.List;

@Data
public class UserVO {

    private Long id;
    private Long orgId;
    private String username;
    private String realName;
    private String nickname;
    private String avatar;
    private String email;
    private String phone;
    private String gender;
    private String status;
    private LocalDateTime expireTime;
    private LocalDateTime lastLoginTime;
    private String lastLoginIp;
    private String remark;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
    private List<Long> roleIds;

    public static UserVO from(SysUser user, List<Long> roleIds) {
        UserVO vo = new UserVO();
        vo.setId(user.getId());
        vo.setOrgId(user.getOrgId());
        vo.setUsername(user.getUsername());
        vo.setRealName(user.getRealName());
        vo.setNickname(user.getNickname());
        vo.setAvatar(user.getAvatar());
        vo.setEmail(user.getEmail());
        vo.setPhone(user.getPhone());
        vo.setGender(user.getGender());
        vo.setStatus(user.getStatus());
        vo.setExpireTime(user.getExpireTime());
        vo.setLastLoginTime(user.getLastLoginTime());
        vo.setLastLoginIp(user.getLastLoginIp());
        vo.setRemark(user.getRemark());
        vo.setCreateTime(user.getCreateTime());
        vo.setUpdateTime(user.getUpdateTime());
        vo.setRoleIds(roleIds);
        return vo;
    }
}
