package com.kb.knowledge.category.dto;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

@Data
@AllArgsConstructor
public class CategoryTreeVO {

    private Long id;
    private Long parentId;
    private String name;
    private String code;
    private Integer sort;
    private String status;
    private List<CategoryTreeVO> children;
}
