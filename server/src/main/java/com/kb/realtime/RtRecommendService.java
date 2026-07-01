package com.kb.realtime;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.ai.AiClient;
import com.kb.ai.dto.AiCitation;
import com.kb.ai.dto.AiQaResponse;
import com.kb.realtime.entity.RtSession;
import com.kb.realtime.entity.RtTranscript;
import com.kb.realtime.mapper.RtSessionMapper;
import com.kb.realtime.mapper.RtTranscriptMapper;
import com.kb.realtime.ws.RtConnectionHub;
import com.fasterxml.jackson.databind.ObjectMapper;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

/**
 * 定时推荐循环：每 5 秒扫描全部 ACTIVE 实时会话，把 last_check_time 之后的新 customer 转写拼接，
 * 复用 {@link AiClient#qa}（/ai/qa，RAG 语义检索）取 Top3 推荐，转 JSON 推给坐席面板。
 * 无新转写或无在线连接则跳过；Python 不可用则推空 articles（不阻断转写展示，设计 §8）。
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class RtRecommendService {

    /** 推荐触发间隔（毫秒）。MVP 硬编码（设计 §4.1），后续迭代再接 rt_config 运行时配置。 */
    static final int RECOMMEND_INTERVAL_MS = 5000;
    /** 每次推荐条数。 */
    static final int RECOMMEND_TOP_K = 3;
    /** 知识类型：MVP 只对话术库做检索（与 QaService.answerCore 默认行为一致）。 */
    static final String KNOWLEDGE_TYPE = "";

    private final RtSessionMapper rtSessionMapper;
    private final RtTranscriptMapper transcriptMapper;
    private final AiClient aiClient;
    private final RtConnectionHub hub;
    private final ObjectMapper objectMapper;

    @Scheduled(fixedDelay = RECOMMEND_INTERVAL_MS)
    public void triggerRecommendations() {
        List<RtSession> active = rtSessionMapper.selectList(Wrappers.<RtSession>lambdaQuery()
                .eq(RtSession::getStatus, "ACTIVE"));
        if (active.isEmpty()) {
            return;
        }
        for (RtSession s : active) {
            try {
                recommendFor(s);
            } catch (Exception e) {
                log.warn("RT 推荐 sessionId={} 失败（不影响会话）: {}", s.getSessionId(), e.toString());
            }
        }
    }

    private void recommendFor(RtSession s) {
        LocalDateTime lastCheck = s.getLastCheckTime() != null ? s.getLastCheckTime()
                : LocalDateTime.of(1970, 1, 1, 0, 0);
        List<RtTranscript> news = transcriptMapper.selectList(Wrappers.<RtTranscript>lambdaQuery()
                .eq(RtTranscript::getSessionId, s.getSessionId())
                .eq(RtTranscript::getSpeaker, "customer")
                .gt(RtTranscript::getCreateTime, lastCheck)
                .orderByAsc(RtTranscript::getSeqNo));
        LocalDateTime now = LocalDateTime.now();
        if (news.isEmpty()) {
            // 无新转写也推进水位线，避免老转写被反复检索
            s.setLastCheckTime(now);
            rtSessionMapper.updateById(s);
            return;
        }
        // 若无在线连接：只推进水位线，跳过昂贵的 Python 调用
        if (!hub.hasConnection(s.getSessionId())) {
            s.setLastCheckTime(now);
            rtSessionMapper.updateById(s);
            return;
        }

        String trigger = news.stream().map(RtTranscript::getContent)
                .map(String::trim).filter(c -> !c.isEmpty())
                .collect(Collectors.joining(" "));
        if (trigger.isEmpty()) {
            s.setLastCheckTime(now);
            rtSessionMapper.updateById(s);
            return;
        }

        List<Map<String, Object>> articles = new ArrayList<>();
        long maxSeq = news.get(news.size() - 1).getSeqNo();
        try {
            AiQaResponse resp = aiClient.qa(trigger, KNOWLEDGE_TYPE, RECOMMEND_TOP_K, null);
            if (resp != null && resp.getCitations() != null) {
                int rank = 1;
                for (AiCitation c : resp.getCitations()) {
                    Map<String, Object> a = new HashMap<>();
                    a.put("articleId", c.getArticleId());
                    a.put("title", c.getTitle());
                    a.put("score", c.getScore());
                    a.put("rank", rank++);
                    articles.add(a);
                }
            }
        } catch (Exception e) {
            log.warn("RT 推荐 Python 调用失败 sessionId={}（推空）: {}", s.getSessionId(), e.toString());
        }

        Map<String, Object> data = new HashMap<>();
        data.put("triggerText", truncate(trigger, 200));
        data.put("articles", articles);
        data.put("lastSeqNo", maxSeq);
        hub.send(s.getSessionId(), envelope("recommendation", data));

        s.setLastCheckTime(now);
        rtSessionMapper.updateById(s);
    }

    private String envelope(String type, Object data) {
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

    private static String truncate(String s, int max) {
        return s == null ? "" : (s.length() <= max ? s : s.substring(0, max));
    }
}