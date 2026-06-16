package com.kb.interaction.dto;

import lombok.Data;

/** 收藏入参。 */
@Data
public class FavoriteRequest {
    private Long articleId;
    /** 收藏夹名（可选） */
    private String folder;
}
