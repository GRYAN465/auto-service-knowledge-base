package com.kb.ai;

import com.kb.ai.dto.FeedbackRequest;
import com.kb.ai.dto.QaMessageVO;
import com.kb.ai.dto.QaRequest;
import com.kb.ai.dto.QaSessionVO;
import com.kb.ai.dto.QaVO;
import com.kb.common.Result;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

/**
 * 智能问答（模块 9，权限码 {@code ai:qa}）：提问 + 答案反馈 + 会话历史。
 * 受 JWT 保护（{@code SecurityConfig} 默认 authenticated），且仅操作当前登录用户自己的数据。
 */
@Tag(name = "智能问答")
@RestController
@RequestMapping("/ai/qa")
public class QaController {

    private final QaService qaService;

    public QaController(QaService qaService) {
        this.qaService = qaService;
    }

    @Operation(summary = "提问（检索增强问答）")
    @PostMapping
    @PreAuthorize("hasAuthority('ai:qa')")
    public Result<QaVO> ask(@Valid @RequestBody QaRequest request) {
        return Result.ok(qaService.ask(request.getQuestion(), request.getTopK(),
                request.getCategoryId(), request.getSessionId(), request.getKnowledgeType()));
    }

    @Operation(summary = "答案反馈（赞/踩）")
    @PostMapping("/feedback")
    @PreAuthorize("hasAuthority('ai:qa')")
    public Result<Void> feedback(@Valid @RequestBody FeedbackRequest request) {
        qaService.feedback(request);
        return Result.ok();
    }

    @Operation(summary = "我的会话列表")
    @GetMapping("/sessions")
    @PreAuthorize("hasAuthority('ai:qa')")
    public Result<List<QaSessionVO>> sessions() {
        return Result.ok(qaService.listSessions());
    }

    @Operation(summary = "会话历史消息")
    @GetMapping("/sessions/{id}/messages")
    @PreAuthorize("hasAuthority('ai:qa')")
    public Result<List<QaMessageVO>> messages(@PathVariable Long id) {
        return Result.ok(qaService.listMessages(id));
    }
}
