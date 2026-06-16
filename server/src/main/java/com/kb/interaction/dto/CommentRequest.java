package com.kb.interaction.dto;

import lombok.Data;

/** 评论入参。parentId=0 或 null 为顶层评论，否则为对该父评论的回复。 */
@Data
public class CommentRequest {
    private Long articleId;
    private Long parentId;
    private String content;
}
