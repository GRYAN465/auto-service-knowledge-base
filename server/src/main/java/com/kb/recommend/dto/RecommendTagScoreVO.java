package com.kb.recommend.dto;

import lombok.Data;

/** 兴趣标签得分。 */
@Data
public class RecommendTagScoreVO {

    private Long id;
    private String name;
    private Double score;
}
