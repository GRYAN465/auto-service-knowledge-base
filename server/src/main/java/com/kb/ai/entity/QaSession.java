package com.kb.ai.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * 问答会话（qa_session）。一个用户的一串提问归一会话，含审计与软删。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("qa_session")
public class QaSession extends BaseEntity {

    private Long userId;
    /** 会话标题（首问截断） */
    private String title;
    private Integer messageCount;
}
