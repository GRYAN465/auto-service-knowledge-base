package com.kb.knowledge.tag.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * 知识标签（表 kb_tag，扁平、name 唯一）。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("kb_tag")
public class KbTag extends BaseEntity {

    private String name;
    private String color;
    private Integer sort;
}
