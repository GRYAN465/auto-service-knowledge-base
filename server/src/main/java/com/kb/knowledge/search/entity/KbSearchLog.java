package com.kb.knowledge.search.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 检索埋点（kb_search_log）。每次检索写一条（含空关键词的浏览式检索），不继承 BaseEntity，仅 createTime。
 */
@Data
@TableName("kb_search_log")
public class KbSearchLog {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long userId;
    private String keyword;
    private Integer resultCount;
    private String clientIp;
    private LocalDateTime createTime;
}
