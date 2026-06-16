package com.kb.knowledge.search;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.knowledge.article.dto.ArticleListItemVO;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

/**
 * 知识门户检索（模块 5，路由权限 {@code knowledge:search}）。
 * 关键词全文检索 + 分类/标签筛选，仅返回 ONLINE 知识，分页。
 */
@Tag(name = "知识检索")
@RestController
@RequestMapping("/search/article")
public class SearchController {

    private final SearchService searchService;

    public SearchController(SearchService searchService) {
        this.searchService = searchService;
    }

    @Operation(summary = "知识检索（全文 + 分类/标签筛选，仅 ONLINE）")
    @GetMapping
    @PreAuthorize("hasAuthority('knowledge:search')")
    public Result<PageResult<ArticleListItemVO>> search(@RequestParam(defaultValue = "1") long page,
                                                        @RequestParam(defaultValue = "20") long pageSize,
                                                        @RequestParam(required = false) String keyword,
                                                        @RequestParam(required = false) Long categoryId,
                                                        @RequestParam(required = false) Long tagId,
                                                        HttpServletRequest http) {
        return Result.ok(searchService.search(page, pageSize, keyword, categoryId, tagId, clientIp(http)));
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
