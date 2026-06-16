package com.kb.system.permission.dto;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

@Data
@AllArgsConstructor
public class PermissionTreeVO {

    private Long id;
    private Long parentId;
    private String name;
    private String type;
    private String permissionCode;
    private String route;
    private String icon;
    private Integer sort;
    private String status;
    private List<PermissionTreeVO> children;
}
