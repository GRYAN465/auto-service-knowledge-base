package com.kb.knowledge.article.dto;

import lombok.Data;

/**
 * 标签简要信息（详情页展示用）。
 */
@Data
public class TagBriefVO {

    private Long id;
    private String name;
    private String color;

    public TagBriefVO() {
    }

    public TagBriefVO(Long id, String name, String color) {
        this.id = id;
        this.name = name;
        this.color = color;
    }
}
