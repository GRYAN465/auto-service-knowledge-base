package com.kb.interaction.dto;

import lombok.Data;

import java.time.LocalDateTime;

/** 我的收藏列表项（富集知识标题/分类名）。 */
@Data
public class FavoriteVO {
    private Long id;
    private Long articleId;
    private String title;
    private String categoryName;
    private String knowledgeType;
    private String folder;
    private LocalDateTime createTime;
}
