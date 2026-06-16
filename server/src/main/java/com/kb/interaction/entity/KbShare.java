package com.kb.interaction.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 分享（kb_share）。一条分享即对收件人的站内通知（read_status 0未读/1已读）。
 * 不继承 BaseEntity，仅 createTime。
 */
@Data
@TableName("kb_share")
public class KbShare {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Long fromUserId;
    private Long toUserId;
    /** USER（站内分享给同事） */
    private String shareType;
    private String message;
    /** 0未读 / 1已读 */
    private Integer readStatus;
    private LocalDateTime createTime;
}
