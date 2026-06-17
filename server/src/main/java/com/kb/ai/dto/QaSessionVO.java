package com.kb.ai.dto;

import lombok.Data;

import java.time.LocalDateTime;

/**
 * 会话列表项（camelCase）。
 */
@Data
public class QaSessionVO {

    private Long id;
    private String title;
    private Integer messageCount;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
