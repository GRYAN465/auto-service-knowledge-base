package com.kb.interaction.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 点赞/点踩（kb_like）。唯一 (target_type,target_id,user_id)。
 * target_type ARTICLE/COMMENT；type 1赞/-1踩。不继承 BaseEntity，仅 createTime。
 */
@Data
@TableName("kb_like")
public class KbLike {

    @TableId(type = IdType.AUTO)
    private Long id;

    /** ARTICLE / COMMENT */
    private String targetType;
    private Long targetId;
    private Long userId;
    /** 1赞 / -1踩 */
    private Integer type;
    private LocalDateTime createTime;
}
