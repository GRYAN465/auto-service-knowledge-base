package com.kb.interaction.dto;

import lombok.Data;

/** 分享入参。把某知识分享给同事 toUserId，附留言 message。 */
@Data
public class ShareRequest {
    private Long articleId;
    private Long toUserId;
    private String message;
}
