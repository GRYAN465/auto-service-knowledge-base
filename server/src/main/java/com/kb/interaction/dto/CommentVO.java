package com.kb.interaction.dto;

import lombok.Data;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;

/** 评论树节点。parentId=0 为顶层，children 为其下回复。 */
@Data
public class CommentVO {
    private Long id;
    private Long articleId;
    private Long userId;
    private String userName;
    private Long parentId;
    private String content;
    private Long likeCount;
    private LocalDateTime createTime;
    private List<CommentVO> children = new ArrayList<>();
}
