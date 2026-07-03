package com.kb.realtime;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.common.Result;
import com.kb.realtime.entity.RtSession;
import com.kb.realtime.entity.RtTranscript;
import com.kb.realtime.mapper.RtTranscriptMapper;
import com.kb.realtime.ws.RtConnectionHub;
import com.fasterxml.jackson.databind.ObjectMapper;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import jakarta.validation.constraints.NotBlank;
import lombok.Data;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * 客服实时辅助（模块 10，权限码 {@code realtime:assist}）：会话生命周期 + ASR 推送 + 转写回放。
 * WebSocket 推送在 ws/RtWebSocketConfig；定时推荐在 RtRecommendService。
 */
@Tag(name = "客服实时辅助")
@RestController
@RequestMapping("/realtime")
@RequiredArgsConstructor
@Slf4j
public class RtController {

    private final RtSessionService sessionService;
    private final RtAsrService asrService;
    private final RtTranscriptMapper transcriptMapper;
    private final RtRecommendService recommendService;
    private final RtConnectionHub hub;
    private final ObjectMapper objectMapper;

    @Operation(summary = "开始会话")
    @PostMapping("/session/start")
    @PreAuthorize("hasAuthority('realtime:assist')")
    public Result<Map<String, Object>> start(@Valid @RequestBody StartRequest req) {
        return Result.ok(sessionService.start(req.callerNumber));
    }

    @Operation(summary = "结束会话")
    @PostMapping("/session/end")
    @PreAuthorize("hasAuthority('realtime:assist')")
    public Result<Void> end(@Valid @RequestBody EndRequest req) {
        sessionService.end(req.sessionId);
        // 通知坐席面板会话已结束
        Map<String, Object> data = new HashMap<>();
        data.put("sessionId", req.sessionId);
        hub.send(req.sessionId, envelope("session_end", data));
        return Result.ok();
    }

    @Operation(summary = "查询当前坐席活跃会话（断线重连）")
    @GetMapping("/session/active")
    @PreAuthorize("hasAuthority('realtime:assist')")
    public Result<Map<String, Object>> active() {
        RtSession rt = sessionService.activeOf(com.kb.security.SecurityUtils.getUserIdOrNull());
        if (rt == null) {
            return Result.ok(null);
        }
        Map<String, Object> data = new HashMap<>();
        data.put("sessionId", rt.getSessionId());
        data.put("callerNumber", rt.getCallerNumber());
        data.put("status", rt.getStatus());
        return Result.ok(data);
    }

    @Operation(summary = "推送 ASR 转写片段")
    @PostMapping("/asr/push")
    @PreAuthorize("hasAuthority('realtime:assist')")
    public Result<Map<String, Object>> push(@Valid @RequestBody AsrPushRequest req) {
        int seqNo = asrService.push(req.sessionId, req.speaker, req.content);
        return Result.ok(Map.of("sessionId", req.sessionId, "seqNo", seqNo));
    }

    @Operation(summary = "查询会话全部转写（断线重连恢复）")
    @GetMapping("/transcript")
    @PreAuthorize("hasAuthority('realtime:assist')")
    public Result<Map<String, Object>> transcript(@RequestParam Long sessionId) {
        List<Map<String, Object>> items = transcriptMapper.selectList(Wrappers.<RtTranscript>lambdaQuery()
                        .eq(RtTranscript::getSessionId, sessionId)
                        .orderByAsc(RtTranscript::getSeqNo))
                .stream().map(t -> {
                    Map<String, Object> m = new HashMap<>();
                    m.put("speaker", t.getSpeaker());
                    m.put("content", t.getContent());
                    m.put("seqNo", t.getSeqNo());
                    return m;
                }).toList();
        return Result.ok(Map.of("items", items));
    }

    @Operation(summary = "按转写句即时取推荐知识（点击某句客户话术触发）")
    @PostMapping("/recommend/by-seq")
    @PreAuthorize("hasAuthority('realtime:assist')")
    public Result<Map<String, Object>> recommendBySeq(@Valid @RequestBody RecommendBySeqRequest req) {
        // 按 sessionId+seqNo 取权威 content（避免前端文本转义/截断），复用定时推荐同一检索路径
        RtTranscript t = transcriptMapper.selectOne(Wrappers.<RtTranscript>lambdaQuery()
                .eq(RtTranscript::getSessionId, req.getSessionId())
                .eq(RtTranscript::getSeqNo, req.getSeqNo()));
        if (t == null) {
            return Result.ok(Map.of("triggerText", "", "articles", List.of()));
        }
        List<Map<String, Object>> articles = recommendService.recommendForText(t.getContent());
        Map<String, Object> data = new HashMap<>();
        data.put("triggerText", truncate(t.getContent(), 200));
        data.put("articles", articles);
        return Result.ok(data);
    }

    private String envelope(String type, Object data) {
        try {
            Map<String, Object> msg = new HashMap<>();
            msg.put("type", type);
            msg.put("data", data);
            return objectMapper.writeValueAsString(msg);
        } catch (Exception e) {
            return "{}";
        }
    }

    private static String truncate(String s, int max) {
        return s == null ? "" : (s.length() <= max ? s : s.substring(0, max));
    }

    @Data
    public static class StartRequest {
        /** 主叫号码（可空，仅展示用） */
        private String callerNumber;
    }

    @Data
    public static class EndRequest {
        @jakarta.validation.constraints.NotNull
        private Long sessionId;
    }

    @Data
    public static class AsrPushRequest {
        @jakarta.validation.constraints.NotNull
        private Long sessionId;
        @NotBlank
        private String speaker;
        @NotBlank
        private String content;
    }

    @Data
    public static class RecommendBySeqRequest {
        @jakarta.validation.constraints.NotNull
        private Long sessionId;
        @jakarta.validation.constraints.NotNull
        private Integer seqNo;
    }
}