package com.kb.knowledge.article;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.knowledge.article.dto.ArticleDetailVO;
import com.kb.knowledge.article.dto.ArticleListItemVO;
import com.kb.knowledge.article.dto.ArticleRequest;
import com.kb.knowledge.article.dto.AuditRequest;
import com.kb.knowledge.article.dto.OfflineRequest;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.entity.KbArticleVersion;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.Valid;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@Tag(name = "知识管理-知识")
@RestController
@RequestMapping("/knowledge/article")
public class ArticleController {

    private final ArticleService articleService;

    public ArticleController(ArticleService articleService) {
        this.articleService = articleService;
    }

    @Operation(summary = "知识分页（标题/分类/标签/状态/类型）")
    @GetMapping
    @PreAuthorize("hasAuthority('knowledge:article:list')")
    public Result<PageResult<ArticleListItemVO>> page(@RequestParam(defaultValue = "1") long page,
                                                      @RequestParam(defaultValue = "20") long pageSize,
                                                      @RequestParam(required = false) String keyword,
                                                      @RequestParam(required = false) Long categoryId,
                                                      @RequestParam(required = false) Long tagId,
                                                      @RequestParam(required = false) String status,
                                                      @RequestParam(required = false) String knowledgeType) {
        return Result.ok(articleService.page(page, pageSize, keyword, categoryId, tagId, status, knowledgeType));
    }

    @Operation(summary = "知识详情（正文 + 标签 + 附件）")
    @GetMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:article:list')")
    public Result<ArticleDetailVO> detail(@PathVariable Long id) {
        return Result.ok(articleService.detail(id));
    }

    @Operation(summary = "版本列表")
    @GetMapping("/{id}/versions")
    @PreAuthorize("hasAuthority('knowledge:article:list')")
    public Result<List<KbArticleVersion>> versions(@PathVariable Long id) {
        return Result.ok(articleService.versions(id));
    }

    @Operation(summary = "查看详情并埋点浏览（门户）")
    @GetMapping("/{id}/view")
    @PreAuthorize("hasAuthority('knowledge:search')")
    public Result<ArticleDetailVO> view(@PathVariable Long id, HttpServletRequest http) {
        return Result.ok(articleService.viewDetail(id, clientIp(http)));
    }

    @Operation(summary = "新建知识（草稿）")
    @PostMapping
    @PreAuthorize("hasAuthority('knowledge:article:create')")
    public Result<KbArticle> create(@RequestBody @Valid ArticleRequest request) {
        return Result.ok(articleService.create(request));
    }

    @Operation(summary = "编辑知识（生成新版本）")
    @PutMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:article:update')")
    public Result<KbArticle> update(@PathVariable Long id, @RequestBody @Valid ArticleRequest request) {
        return Result.ok(articleService.update(id, request));
    }

    @Operation(summary = "删除知识（逻辑删除）")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:article:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        articleService.delete(id);
        return Result.ok();
    }

    @Operation(summary = "提交审核")
    @PostMapping("/{id}/submit")
    @PreAuthorize("hasAuthority('knowledge:article:submit')")
    public Result<Void> submit(@PathVariable Long id) {
        articleService.submit(id);
        return Result.ok();
    }

    @Operation(summary = "审核（PASS/REJECT）")
    @PostMapping("/{id}/audit")
    @PreAuthorize("hasAuthority('knowledge:article:audit')")
    public Result<Void> audit(@PathVariable Long id, @RequestBody @Valid AuditRequest request) {
        articleService.audit(id, request);
        return Result.ok();
    }

    @Operation(summary = "上线")
    @PostMapping("/{id}/online")
    @PreAuthorize("hasAuthority('knowledge:article:online')")
    public Result<Void> online(@PathVariable Long id) {
        articleService.online(id);
        return Result.ok();
    }

    @Operation(summary = "下线")
    @PostMapping("/{id}/offline")
    @PreAuthorize("hasAuthority('knowledge:article:offline')")
    public Result<Void> offline(@PathVariable Long id, @RequestBody(required = false) OfflineRequest request) {
        articleService.offline(id, request);
        return Result.ok();
    }

    /** 取客户端 IP（优先 X-Forwarded-For 首段），与 AuthController 同款。 */
    private static String clientIp(HttpServletRequest request) {
        String xff = request.getHeader("X-Forwarded-For");
        if (xff != null && !xff.isBlank()) {
            int comma = xff.indexOf(',');
            return comma > 0 ? xff.substring(0, comma).trim() : xff.trim();
        }
        return request.getRemoteAddr();
    }
}
