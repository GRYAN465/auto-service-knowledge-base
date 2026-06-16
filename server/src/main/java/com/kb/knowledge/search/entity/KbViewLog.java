package com.kb.knowledge.search.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 知识浏览埋点（kb_view_log）。每次查看详情写一条，不继承 BaseEntity，仅 createTime。
 */
@Data
@TableName("kb_view_log")
public class KbViewLog {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Long userId;
    private String clientIp;
    private Integer staySeconds;
    private LocalDateTime createTime;
}
