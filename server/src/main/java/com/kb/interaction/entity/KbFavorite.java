package com.kb.interaction.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 收藏（kb_favorite）。唯一 (article_id,user_id)。不继承 BaseEntity，仅 createTime（镜像 KbViewLog）。
 */
@Data
@TableName("kb_favorite")
public class KbFavorite {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Long userId;
    private String folder;
    private LocalDateTime createTime;
}
