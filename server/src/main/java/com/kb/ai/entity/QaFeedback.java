package com.kb.ai.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 问答答案反馈（qa_feedback）。每用户每消息一条（uk_qa_fb 冲突即更新）。
 */
@Data
@TableName("qa_feedback")
public class QaFeedback {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long messageId;
    private Long userId;
    /** 1赞/0踩 */
    private Integer helpful;
    private String reason;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
