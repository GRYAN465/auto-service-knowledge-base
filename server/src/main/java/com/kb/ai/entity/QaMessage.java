package com.kb.ai.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 问答消息（qa_message）。一问一答一行，引用与反馈挂其上。
 */
@Data
@TableName("qa_message")
public class QaMessage {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long sessionId;
    private Long userId;
    private String question;
    private String answer;
    /** llm/extractive/no_hit */
    private String mode;
    private Integer topK;
    private LocalDateTime createTime;
}
