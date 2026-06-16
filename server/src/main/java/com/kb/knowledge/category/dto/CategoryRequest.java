package com.kb.knowledge.category.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class CategoryRequest {

    private Long parentId = 0L;

    @NotBlank(message = "分类名称不能为空")
    private String name;

    private String code;
    private Integer sort = 0;
    private String status = "ENABLED";
}
