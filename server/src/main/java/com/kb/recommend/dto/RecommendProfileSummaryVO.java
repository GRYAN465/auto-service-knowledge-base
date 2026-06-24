package com.kb.recommend.dto;

import lombok.Data;

import java.util.List;

/** 用户兴趣画像摘要。 */
@Data
public class RecommendProfileSummaryVO {

    private List<RecommendTagScoreVO> topTags;
    private List<String> keywords;
    /** vector / fallback */
    private String source;
}
