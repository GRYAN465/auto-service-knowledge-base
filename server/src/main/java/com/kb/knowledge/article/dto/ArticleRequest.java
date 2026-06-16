package com.kb.knowledge.article.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

import java.util.List;

/**
 * 新建/编辑知识入参。content 为富文本 HTML，tagIds 为挂接标签。
 */
@Data
public class ArticleRequest {

    @NotBlank(message = "标题不能为空")
    private String title;

    private Long categoryId;
    /** SCRIPT/TRAIN/PRODUCT/OFFICE */
    private String knowledgeType;
    private String summary;
    private String content;
    private List<Long> tagIds;
    /** 编辑时的版本变更说明 */
    private String changeLog;
}
