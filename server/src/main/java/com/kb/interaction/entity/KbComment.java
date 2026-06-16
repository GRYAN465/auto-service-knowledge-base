package com.kb.interaction.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableLogic;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 评论（kb_comment）。parentId=0 为顶层、其余为回复；status NORMAL/HIDDEN。
 * 该表有软删列 deleted 但无 create_by/update_by/update_time，故不继承 BaseEntity，
 * 仅手写 createTime 与 @TableLogic deleted。
 */
@Data
@TableName("kb_comment")
public class KbComment {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Long userId;
    /** 回复的父评论，0 为顶层 */
    private Long parentId;
    private String content;
    /** NORMAL / HIDDEN */
    private String status;
    private Long likeCount;
    private LocalDateTime createTime;

    @TableLogic
    private Integer deleted;
}
