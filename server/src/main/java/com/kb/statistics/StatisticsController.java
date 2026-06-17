package com.kb.statistics;

import com.kb.common.Result;
import com.kb.statistics.dto.HotArticleVO;
import com.kb.statistics.dto.HotKeywordVO;
import com.kb.statistics.dto.OverviewVO;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

/**
 * 数据统计（模块 7）：仪表盘汇总 / 热门知识 / 热门搜索词。
 *
 * <p>全部为后台分析视图，统一权限码 {@code statistics:view}（菜单 401「数据统计」，V2 已种入）。
 * 只读、GET，不入 {@code sys_operation_log}。
 */
@Tag(name = "数据统计")
@RestController
@RequestMapping("/statistics")
public class StatisticsController {

    private final StatisticsService statisticsService;

    public StatisticsController(StatisticsService statisticsService) {
        this.statisticsService = statisticsService;
    }

    @Operation(summary = "仪表盘汇总")
    @GetMapping("/overview")
    @PreAuthorize("hasAuthority('statistics:view')")
    public Result<OverviewVO> overview() {
        return Result.ok(statisticsService.overview());
    }

    @Operation(summary = "热门知识（按累计浏览量，可选分类含子分类）")
    @GetMapping("/hot-article")
    @PreAuthorize("hasAuthority('statistics:view')")
    public Result<List<HotArticleVO>> hotArticle(@RequestParam(required = false) Long categoryId,
                                                 @RequestParam(defaultValue = "10") int limit) {
        return Result.ok(statisticsService.hotArticles(categoryId, limit));
    }

    @Operation(summary = "热门搜索词（可选最近 days 天时间窗）")
    @GetMapping("/hot-keyword")
    @PreAuthorize("hasAuthority('statistics:view')")
    public Result<List<HotKeywordVO>> hotKeyword(@RequestParam(defaultValue = "30") int days,
                                                 @RequestParam(defaultValue = "10") int limit) {
        return Result.ok(statisticsService.hotKeywords(days, limit));
    }
}
