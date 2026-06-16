package com.kb.knowledge.article.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 知识审核记录（kb_audit_record）。提交与审核各写一条留痕，不继承 BaseEntity。
 */
@Data
@TableName("kb_audit_record")
public class KbAuditRecord {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Long submitBy;
    private Long auditorId;
    /** PASS/REJECT */
    private String auditStatus;
    private String auditOpinion;
    private LocalDateTime submitTime;
    private LocalDateTime auditTime;
}
