package com.kb.interaction.dto;

import lombok.Data;

/**
 * 某知识对当前用户的互动初态，供详情弹窗加载与每次互动后刷新。
 * myLikeType：1 我已赞 / -1 我已踩 / 0 未表态。
 */
@Data
public class InteractionStateVO {
    private boolean favorited;
    private Integer myLikeType;
    private Long likeCount;
    private Long dislikeCount;
    private Long favoriteCount;
    private Long commentCount;
}
