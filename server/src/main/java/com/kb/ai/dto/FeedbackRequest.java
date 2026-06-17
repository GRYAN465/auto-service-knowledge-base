package com.kb.ai.dto;

import jakarta.validation.constraints.NotNull;
import lombok.Data;

/**
 * 问答反馈入参。{@code helpful}：1 赞 / 0 踩。同一用户对同一消息只保留一条（冲突即更新）。
 */
@Data
public class FeedbackRequest {

    @NotNull(message = "messageId 不能为空")
    private Long messageId;

    @NotNull(message = "helpful 不能为空")
    private Integer helpful;

    /** 点踩原因（可选）。 */
    private String reason;
}
