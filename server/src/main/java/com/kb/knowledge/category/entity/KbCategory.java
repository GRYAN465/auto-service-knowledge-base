package com.kb.knowledge.category.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * 知识分类（表 kb_category，树结构）。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("kb_category")
public class KbCategory extends BaseEntity {

    private Long parentId;
    private String name;
    private String code;
    private Integer sort;
    private String status;
}
