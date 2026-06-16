package com.kb.system.user.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

import java.time.LocalDateTime;
import java.util.List;

@Data
public class UserRequest {

    private Long orgId;

    @NotBlank(message = "用户名不能为空")
    private String username;

    private String password;
    private String realName;
    private String nickname;
    private String avatar;
    private String email;
    private String phone;
    private String gender = "U";
    private String status = "ENABLED";
    private LocalDateTime expireTime;
    private String remark;
    private List<Long> roleIds;
}
