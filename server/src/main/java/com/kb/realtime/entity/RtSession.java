package com.kb.realtime.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 活跃实时辅助会话（rt_active_session）。sessionId 唯一关联 qa_session；
 * agent_user_id=坐席；status=ACTIVE/ENDED；last_check_time 供定时推荐循环排除已检索转写。
 * 会话结束后保留记录（轻量沉淀），不做软删除。
 */
@Data
@TableName("rt_active_session")
public class RtSession {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long sessionId;
    private Long agentUserId;
    private String callerNumber;
    private String ccCallId;
    /** ACTIVE / ENDED */
    private String status;
    private LocalDateTime lastCheckTime;
    private LocalDateTime createTime;
}