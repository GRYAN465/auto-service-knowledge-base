package com.kb.ai;

import com.kb.common.Result;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Map;

/**
 * 向量索引管理（模块 9，权限码 {@code ai:index}，仅管理员）：把在线知识正文重建进 Chroma。
 * 平时上下线由 {@code ArticleIndexListener} 自动同步，本接口用于初次回填或异常后批量修复。
 */
@Tag(name = "向量索引管理")
@RestController
@RequestMapping("/ai/index")
public class AiIndexController {

    private final VectorIndexService vectorIndexService;

    public AiIndexController(VectorIndexService vectorIndexService) {
        this.vectorIndexService = vectorIndexService;
    }

    @Operation(summary = "重建全部在线知识的向量索引")
    @PostMapping("/rebuild")
    @PreAuthorize("hasAuthority('ai:index')")
    public Result<Map<String, Integer>> rebuildAll() {
        return Result.ok(vectorIndexService.reindexAllOnline());
    }

    @Operation(summary = "重建指定知识的向量索引")
    @PostMapping("/rebuild/{articleId}")
    @PreAuthorize("hasAuthority('ai:index')")
    public Result<String> rebuildOne(@PathVariable Long articleId) {
        return Result.ok(vectorIndexService.reindexArticle(articleId));
    }
}
