package com.kb.recommend.dto;

import com.kb.knowledge.article.dto.TagBriefVO;
import lombok.Data;

import java.util.List;

/** 首页推荐单条知识。 */
@Data
public class RecommendArticleVO {

    private Long id;
    private String title;
    private String categoryName;
    private String knowledgeType;
    private Long viewCount;
    private Long likeCount;
    private Long commentCount;
    private List<TagBriefVO> tags;
    /** 综合匹配分 0~1 */
    private Double matchScore;
    /** 归一化热度分 0~1 */
    private Double hotScore;
    /** 兴趣标签名（展示用） */
    private List<String> reasonTags;
}
