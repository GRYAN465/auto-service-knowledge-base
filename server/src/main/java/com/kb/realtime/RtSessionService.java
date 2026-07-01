package com.kb.realtime;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.ai.entity.QaSession;
import com.kb.ai.mapper.QaSessionMapper;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.realtime.entity.RtSession;
import com.kb.realtime.mapper.RtSessionMapper;
import com.kb.security.SecurityUtils;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Map;

/**
 * 实时辅助会话管理（start/end/active）。会话复用 qa_session，title 前缀「实时辅助-」区分普通问答。
 * 同一坐席已有 ACTIVE 会话时 start 幂等返回已有 sessionId（不重复创建，符合设计 §8）。
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class RtSessionService {

    /** qa_session.title 前缀，用于与普通问答会话区分；查询实时会话以此 LIKE 过滤。 */
    public static final String TITLE_PREFIX = "实时辅助-";
    private static final String STATUS_ACTIVE = "ACTIVE";
    private static final String STATUS_ENDED = "ENDED";
    private static final DateTimeFormatter TITLE_FMT = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm");

    private final RtSessionMapper rtSessionMapper;
    private final QaSessionMapper qaSessionMapper;

    /** 开始会话：同坐席已有 ACTIVE 则返回已有 sessionId。返回 {sessionId, callerNumber}。 */
    public Map<String, Object> start(String callerNumber) {
        Long userId = SecurityUtils.getUserIdOrNull();
        if (userId == null) {
            throw new BusinessException(ResultCode.UNAUTHORIZED, "未登录");
        }
        RtSession active = findActive(userId);
        if (active != null) {
            return Map.of("sessionId", active.getSessionId(), "callerNumber",
                    active.getCallerNumber() == null ? "" : active.getCallerNumber(),
                    "reused", true);
        }
        String title = TITLE_PREFIX + (callerNumber == null ? "" : maskCaller(callerNumber))
                + "-" + LocalDateTime.now().format(TITLE_FMT);
        QaSession qa = new QaSession();
        qa.setUserId(userId);
        qa.setTitle(title);
        qa.setMessageCount(0);
        qaSessionMapper.insert(qa);

        RtSession rt = new RtSession();
        rt.setSessionId(qa.getId());
        rt.setAgentUserId(userId);
        rt.setCallerNumber(callerNumber);
        rt.setStatus(STATUS_ACTIVE);
        rt.setLastCheckTime(LocalDateTime.now());
        rt.setCreateTime(LocalDateTime.now());
        rtSessionMapper.insert(rt);
        log.info("RT 会话开始 sessionId={} agent={} caller={}", qa.getId(), userId, callerNumber);
        return Map.of("sessionId", qa.getId(), "callerNumber", callerNumber == null ? "" : callerNumber,
                "reused", false);
    }

    /** 结束会话：仅更新状态，不删 qa_session（保留转写沉淀）。 */
    public void end(Long sessionId) {
        RtSession rt = rtSessionMapper.selectOne(Wrappers.<RtSession>lambdaQuery()
                .eq(RtSession::getSessionId, sessionId));
        if (rt == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "会话不存在");
        }
        Long userId = SecurityUtils.getUserIdOrNull();
        if (userId != null && !userId.equals(rt.getAgentUserId())) {
            throw new BusinessException(ResultCode.FORBIDDEN, "无权操作他人会话");
        }
        rt.setStatus(STATUS_ENDED);
        rtSessionMapper.updateById(rt);
        log.info("RT 会话结束 sessionId={}", sessionId);
    }

    /** 查当前坐席 ACTIVE 会话（断线重连恢复用），无则返回 null。 */
    public RtSession activeOf(Long agentUserId) {
        return findActive(agentUserId);
    }

    /** 按 sessionId 取活跃会话；非 ACTIVE 返回 null。 */
    public RtSession getActiveBySessionId(Long sessionId) {
        RtSession rt = rtSessionMapper.selectOne(Wrappers.<RtSession>lambdaQuery()
                .eq(RtSession::getSessionId, sessionId));
        return rt != null && STATUS_ACTIVE.equals(rt.getStatus()) ? rt : null;
    }

    private RtSession findActive(Long agentUserId) {
        return rtSessionMapper.selectOne(Wrappers.<RtSession>lambdaQuery()
                .eq(RtSession::getAgentUserId, agentUserId)
                .eq(RtSession::getStatus, STATUS_ACTIVE)
                .last("LIMIT 1"));
    }

    private static String maskCaller(String number) {
        if (number == null || number.length() <= 4) {
            return number == null ? "" : number;
        }
        int len = number.length();
        return number.substring(0, 3) + "****" + number.substring(len - 4);
    }
}