package com.kb.statistics.dto;

import lombok.Data;

/** 分类知识量排行（含累计浏览）。 */
@Data
public class CategoryRankVO {

    private Long categoryId;
    private String categoryName;
    private Long articleCount;
    private Long viewTotal;
}
