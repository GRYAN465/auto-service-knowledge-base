package com.kb.system.auth.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Size;
import lombok.Data;

@Data
public class RegisterRequest {

    @NotNull(message = "请选择所属机构")
    private Long orgId;

    @NotBlank(message = "用户名不能为空")
    @Size(min = 3, max = 64, message = "用户名长度为 3~64 位")
    private String username;

    @NotBlank(message = "密码不能为空")
    @Size(min = 6, max = 64, message = "密码至少 6 位")
    private String password;

    @NotBlank(message = "姓名不能为空")
    private String realName;

    private String nickname;
    private String email;
    private String phone;
    /** M / F / U，默认 U。 */
    private String gender = "U";
}
