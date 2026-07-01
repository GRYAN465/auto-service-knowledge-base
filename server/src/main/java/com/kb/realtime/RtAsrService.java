package com.kb.realtime;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.realtime.entity.RtSession;
import com.kb.realtime.entity.RtTranscript;
import com.kb.realtime.mapper.RtTranscriptMapper;
import com.kb.realtime.ws.RtConnectionHub;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.util.HashMap;
import java.util.Map;

/**
 * ASR 推送处理：写 rt_transcript + 通过 WebSocket 推 transcript 消息到坐席面板。
 * Mock ASR 经 POST /realtime/asr/push 调入；后续真实 CC 接入沿用此入口。
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class RtAsrService {

    private final RtSessionService sessionService;
    private final RtTranscriptMapper transcriptMapper;
    private final RtConnectionHub hub;
    private final ObjectMapper objectMapper;

    /**
     * 推送一段转写。返回该会话当前片段序号（从 0 起）。
     * 无活跃会话抛 1001（无活跃会话，设计 §8）；坐席越权抛 2003。
     */
    public int push(Long sessionId, String speaker, String content) {
        RtSession rt = sessionService.getActiveBySessionId(sessionId);
        if (rt == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "无活跃会话");
        }
        if (!"customer".equalsIgnoreCase(speaker) && !"agent".equalsIgnoreCase(speaker)) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "speaker 非法（应为 customer/agent）");
        }
        int seqNo = nextSeqNo(sessionId);
        RtTranscript t = new RtTranscript();
        t.setSessionId(sessionId);
        t.setSpeaker(speaker.toLowerCase());
        t.setContent(content);
        t.setSeqNo(seqNo);
        t.setCreateTime(java.time.LocalDateTime.now());
        transcriptMapper.insert(t);

        // 实时推送给坐席面板
        Map<String, Object> data = new HashMap<>();
        data.put("speaker", t.getSpeaker());
        data.put("content", content);
        data.put("seqNo", seqNo);
        if (hub.send(sessionId, envelope("transcript", data))) {
            log.debug("RT 转写已推 sessionId={} seq={}", sessionId, seqNo);
        }
        return seqNo;
    }

    private int nextSeqNo(Long sessionId) {
        Long cnt = transcriptMapper.selectCount(Wrappers.<RtTranscript>lambdaQuery()
                .eq(RtTranscript::getSessionId, sessionId));
        return cnt == null ? 0 : cnt.intValue();
    }

    String envelope(String type, Object data) {
        try {
            Map<String, Object> msg = new HashMap<>();
            msg.put("type", type);
            msg.put("data", data);
            return objectMapper.writeValueAsString(msg);
        } catch (Exception e) {
            log.warn("RT 消息序列化失败 type={}: {}", type, e.toString());
            return "{}";
        }
    }
}