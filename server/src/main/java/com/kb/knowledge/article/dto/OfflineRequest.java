package com.kb.knowledge.article.dto;

import lombok.Data;

/**
 * 下线入参：可附下线原因。
 */
@Data
public class OfflineRequest {

    private String reason;
}
