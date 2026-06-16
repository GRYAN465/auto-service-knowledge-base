package com.kb.interaction.dto;

import lombok.Data;

import java.time.LocalDateTime;

/** 分享记录（收件箱/已发出共用）。富集知识标题与收发双方姓名。 */
@Data
public class ShareVO {
    private Long id;
    private Long articleId;
    private String articleTitle;
    private Long fromUserId;
    private String fromUserName;
    private Long toUserId;
    private String toUserName;
    private String message;
    /** 0未读 / 1已读 */
    private Integer readStatus;
    private LocalDateTime createTime;
}
