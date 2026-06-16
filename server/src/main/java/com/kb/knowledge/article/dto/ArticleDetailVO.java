package com.kb.knowledge.article.dto;

import com.kb.knowledge.article.entity.KbAttachment;
import lombok.Data;

import java.time.LocalDateTime;
import java.util.List;

/**
 * 知识详情：article 全字段 + 分类名 + 标签（含色） + 附件列表。
 */
@Data
public class ArticleDetailVO {

    private Long id;
    private String title;
    private Long categoryId;
    private String categoryName;
    private String knowledgeType;
    private String summary;
    private String content;
    private String status;
    private String source;
    private Long authorId;
    private String authorName;
    private Integer currentVersion;
    private LocalDateTime onlineTime;
    private LocalDateTime offlineTime;
    private String offlineReason;
    private Long viewCount;
    private Long likeCount;
    private Long dislikeCount;
    private Long favoriteCount;
    private Long commentCount;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;
    private List<TagBriefVO> tags;
    private List<KbAttachment> attachments;
}
