package com.kb.system.org.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * 机构/部门（表 sys_org）。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("sys_org")
public class SysOrg extends BaseEntity {

    private Long parentId;
    private String name;
    private String code;
    private String type;
    private Integer sort;
    private String status;
}
