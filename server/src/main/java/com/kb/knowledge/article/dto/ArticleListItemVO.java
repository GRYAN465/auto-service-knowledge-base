package com.kb.knowledge.article.dto;

import lombok.Data;

import java.time.LocalDateTime;
import java.util.List;

/**
 * 知识列表行：article 基本字段 + 分类名 + 作者名 + 标签名集合。
 */
@Data
public class ArticleListItemVO {

    private Long id;
    private String title;
    private Long categoryId;
    private String categoryName;
    private String knowledgeType;
    private String status;
    private String source;
    private Long authorId;
    private String authorName;
    private Integer currentVersion;
    private Long viewCount;
    private LocalDateTime updateTime;
    private List<String> tagNames;
}
