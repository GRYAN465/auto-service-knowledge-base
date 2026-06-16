package com.kb.system.permission.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class PermissionRequest {

    private Long parentId = 0L;

    @NotBlank(message = "权限名称不能为空")
    private String name;

    @NotBlank(message = "权限类型不能为空")
    private String type;

    private String permissionCode;
    private String route;
    private String icon;
    private Integer sort = 0;
    private String status = "ENABLED";
}
