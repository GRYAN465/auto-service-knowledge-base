package com.kb.knowledge.search.dto;

import com.kb.knowledge.article.dto.TagBriefVO;
import lombok.Data;

import java.time.LocalDateTime;
import java.util.List;

/**
 * 知识社区信息流条目：检索/个人中心/他人主页共用。
 */
@Data
public class SearchArticleVO {

    private Long id;
    private String title;
    private String summary;
    /** HTML 剥离后正文前 20 字预览。 */
    private String contentPreview;
    private Long viewCount;
    private Long likeCount;
    private Long commentCount;
    private Long authorId;
    private String authorName;
    private LocalDateTime updateTime;
    private List<TagBriefVO> tags;
}
