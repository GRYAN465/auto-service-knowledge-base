package com.kb.interaction.dto;

import lombok.Data;

/** 点赞/点踩入参。targetType ARTICLE/COMMENT；type 1赞/-1踩。 */
@Data
public class LikeRequest {
    private String targetType;
    private Long targetId;
    private Integer type;
}
