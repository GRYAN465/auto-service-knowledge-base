package com.kb.realtime.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 实时辅助转写片段（rt_transcript）。一段通话里每个 customer/agent 转写片段一行，
 * 按 seq_no 排序回放。与 qa_message（一问一答）解耦——转写无问答语义。
 */
@Data
@TableName("rt_transcript")
public class RtTranscript {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long sessionId;
    /** customer / agent */
    private String speaker;
    private String content;
    private Integer seqNo;
    private LocalDateTime createTime;
}