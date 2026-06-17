package com.kb.ai.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 向量化来源文本块（kb_chunk）。正文方案下每篇通常 1 行；重建即整篇删旧插新，不软删。
 */
@Data
@TableName("kb_chunk")
public class KbChunk {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Integer seq;
    private String content;
    private Integer charLen;
    private LocalDateTime createTime;
}
