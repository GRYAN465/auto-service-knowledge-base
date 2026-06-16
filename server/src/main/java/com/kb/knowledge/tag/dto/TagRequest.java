package com.kb.knowledge.tag.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class TagRequest {

    @NotBlank(message = "标签名称不能为空")
    private String name;

    private String color;
    private Integer sort = 0;
}
