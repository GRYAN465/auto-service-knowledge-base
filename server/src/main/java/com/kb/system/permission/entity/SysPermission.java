package com.kb.system.permission.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * 菜单/按钮权限（表 sys_permission，树）。
 * {@code type}=DIR/MENU/BUTTON；{@code route} 为客户端页面 name（MENU 用）；
 * {@code permissionCode} 为权限码（MENU/BUTTON 用，驱动接口与按钮鉴权）。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("sys_permission")
public class SysPermission extends BaseEntity {

    private Long parentId;
    private String name;
    private String type;
    private String permissionCode;
    private String route;
    private String icon;
    private Integer sort;
    private String status;
}
