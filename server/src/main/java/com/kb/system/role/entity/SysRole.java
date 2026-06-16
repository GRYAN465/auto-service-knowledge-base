package com.kb.system.role.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * 角色（表 sys_role）。{@code code} 如 ADMIN/AUDITOR；{@code dataScope} 数据权限预留。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("sys_role")
public class SysRole extends BaseEntity {

    private String name;
    private String code;
    private String dataScope;
    private Integer sort;
    private String status;
    private String remark;
}
