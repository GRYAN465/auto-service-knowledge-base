package com.kb.ai.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 知识向量化状态（kb_vector_index）。每篇一行（article_id 唯一），upsert 维护；
 * 供前端展示「已索引/失败」与重建依据。
 */
@Data
@TableName("kb_vector_index")
public class KbVectorIndex {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Integer chunkCount;
    private Integer dim;
    private String embeddingModel;
    /** INDEXED/REMOVED/FAILED/EMPTY */
    private String status;
    private String errorMsg;
    private LocalDateTime indexedTime;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
}
