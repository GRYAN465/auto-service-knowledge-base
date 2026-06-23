package com.kb.knowledge.search;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.knowledge.search.dto.SearchArticleVO;
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
 * 关键词全文检索 + 分类/标签/作者筛选，仅返回 ONLINE 知识。
 * 社区信息流传 {@code offset} 时使用 offset 分页（首屏 15、下拉 +10）。
 */
@Tag(name = "知识检索")
@RestController
@RequestMapping("/search/article")
public class SearchController {

    private final SearchService searchService;

    public SearchController(SearchService searchService) {
        this.searchService = searchService;
    }

    @Operation(summary = "知识检索（全文 + 分类/标签/作者筛选，仅 ONLINE）")
    @GetMapping
    @PreAuthorize("hasAuthority('knowledge:search')")
    public Result<PageResult<SearchArticleVO>> search(@RequestParam(defaultValue = "1") long page,
                                                      @RequestParam(defaultValue = "20") long pageSize,
                                                      @RequestParam(required = false) Integer offset,
                                                      @RequestParam(required = false) String keyword,
                                                      @RequestParam(required = false) Long categoryId,
                                                      @RequestParam(required = false) Long tagId,
                                                      @RequestParam(required = false) Long authorId,
                                                      @RequestParam(required = false) String sortBy,
                                                      HttpServletRequest http) {
        String ip = clientIp(http);
        if (offset != null) {
            return Result.ok(searchService.searchByOffset(offset, pageSize, keyword, categoryId, tagId,
                    authorId, sortBy, ip));
        }
        return Result.ok(searchService.search(page, pageSize, keyword, categoryId, tagId, authorId, sortBy, ip));
    }

    private static String clientIp(HttpServletRequest request) {
        String xff = request.getHeader("X-Forwarded-For");
        if (xff != null && !xff.isBlank()) {
            int comma = xff.indexOf(',');
            return comma > 0 ? xff.substring(0, comma).trim() : xff.trim();
        }
        return request.getRemoteAddr();
    }
}
