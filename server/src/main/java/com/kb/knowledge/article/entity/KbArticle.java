package com.kb.knowledge.article.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

import java.time.LocalDateTime;

/**
 * 知识主表（kb_article）。状态机：DRAFT/PENDING_AUDIT/ONLINE/OFFLINE/REJECTED。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("kb_article")
public class KbArticle extends BaseEntity {

    private String title;
    private Long categoryId;
    /** SCRIPT/TRAIN/PRODUCT/OFFICE */
    private String knowledgeType;
    private String summary;
    /** 富文本正文 HTML */
    private String content;
    /** DRAFT/PENDING_AUDIT/ONLINE/OFFLINE/REJECTED */
    private String status;
    /** MANUAL/IMPORT */
    private String source;
    private Long authorId;
    private Integer currentVersion;
    private LocalDateTime onlineTime;
    private LocalDateTime offlineTime;
    private String offlineReason;
    private Long viewCount;
    private Long likeCount;
    private Long dislikeCount;
    private Long favoriteCount;
    private Long commentCount;
}
