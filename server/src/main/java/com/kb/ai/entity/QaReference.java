package com.kb.ai.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.math.BigDecimal;
import java.time.LocalDateTime;

/**
 * 问答答案引用（qa_reference）。命中知识来源；标题由 Java 用 kb_article 回填，分数/片段来自 Python。
 */
@Data
@TableName("qa_reference")
public class QaReference {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long messageId;
    private Long articleId;
    private String title;
    private BigDecimal score;
    private String snippet;
    private Integer sort;
    private LocalDateTime createTime;
}
