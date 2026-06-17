package com.kb.statistics;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.category.CategoryService;
import com.kb.knowledge.category.entity.KbCategory;
import com.kb.knowledge.category.mapper.KbCategoryMapper;
import com.kb.knowledge.search.entity.KbSearchLog;
import com.kb.knowledge.search.entity.KbViewLog;
import com.kb.knowledge.search.mapper.KbSearchLogMapper;
import com.kb.knowledge.search.mapper.KbViewLogMapper;
import com.kb.knowledge.tag.mapper.KbTagMapper;
import com.kb.statistics.dto.ArticleTotalsVO;
import com.kb.statistics.dto.HotArticleVO;
import com.kb.statistics.dto.HotKeywordVO;
import com.kb.statistics.dto.OverviewVO;
import com.kb.statistics.mapper.StatisticsMapper;
import org.springframework.stereotype.Service;

import java.time.LocalDate;
import java.time.LocalDateTime;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * 数据统计业务（模块 7）：仪表盘汇总 / 热门知识 / 热门搜索词。全部只读聚合。
 *
 * <p>聚合分两类：标量计数与「今日」量走 MyBatis-Plus 的 {@code selectCount}（自动应用逻辑删除）；
 * GROUP BY / SUM 类走 {@link StatisticsMapper} 的原生 {@code @Select}（已手动带 deleted=0）。
 * 「按分类」筛选一律经 {@link CategoryService#subtreeIds} 展开为「自身 + 全部子孙」，与检索/管理列表一致。
 */
@Service
public class StatisticsService {

    private static final String ONLINE = "ONLINE";
    private static final String PENDING_AUDIT = "PENDING_AUDIT";
    private static final String DRAFT = "DRAFT";
    private static final String OFFLINE = "OFFLINE";

    private final KbArticleMapper articleMapper;
    private final KbCategoryMapper categoryMapper;
    private final KbTagMapper tagMapper;
    private final KbViewLogMapper viewLogMapper;
    private final KbSearchLogMapper searchLogMapper;
    private final StatisticsMapper statisticsMapper;
    private final CategoryService categoryService;

    public StatisticsService(KbArticleMapper articleMapper, KbCategoryMapper categoryMapper,
                             KbTagMapper tagMapper, KbViewLogMapper viewLogMapper,
                             KbSearchLogMapper searchLogMapper, StatisticsMapper statisticsMapper,
                             CategoryService categoryService) {
        this.articleMapper = articleMapper;
        this.categoryMapper = categoryMapper;
        this.tagMapper = tagMapper;
        this.viewLogMapper = viewLogMapper;
        this.searchLogMapper = searchLogMapper;
        this.statisticsMapper = statisticsMapper;
        this.categoryService = categoryService;
    }

    // ---------------------------------------------------------------- 仪表盘汇总

    public OverviewVO overview() {
        OverviewVO vo = new OverviewVO();

        vo.setArticleTotal(articleMapper.selectCount(null));
        vo.setArticleOnline(countByStatus(ONLINE));
        vo.setArticlePendingAudit(countByStatus(PENDING_AUDIT));
        vo.setArticleDraft(countByStatus(DRAFT));
        vo.setArticleOffline(countByStatus(OFFLINE));

        ArticleTotalsVO totals = statisticsMapper.articleTotals();
        vo.setViewTotal(totals == null ? 0L : totals.getViewTotal());
        vo.setLikeTotal(totals == null ? 0L : totals.getLikeTotal());
        vo.setFavoriteTotal(totals == null ? 0L : totals.getFavoriteTotal());
        vo.setCommentTotal(totals == null ? 0L : totals.getCommentTotal());

        vo.setCategoryCount(categoryMapper.selectCount(null));
        vo.setTagCount(tagMapper.selectCount(null));

        LocalDateTime startOfDay = LocalDate.now().atStartOfDay();
        vo.setTodayViews(viewLogMapper.selectCount(Wrappers.<KbViewLog>lambdaQuery()
                .ge(KbViewLog::getCreateTime, startOfDay)));
        vo.setTodaySearches(searchLogMapper.selectCount(Wrappers.<KbSearchLog>lambdaQuery()
                .ge(KbSearchLog::getCreateTime, startOfDay)));
        vo.setTodayNewArticles(articleMapper.selectCount(Wrappers.<KbArticle>lambdaQuery()
                .ge(KbArticle::getCreateTime, startOfDay)));

        vo.setStatusDist(statisticsMapper.articleStatusDist());
        vo.setTypeDist(statisticsMapper.articleTypeDist());
        return vo;
    }

    // ---------------------------------------------------------------- 热门知识

    /**
     * 热门知识：按累计浏览量倒序的 ONLINE 知识（门户可见者才算热门）。
     * {@code categoryId} 非空则展开为「自身 + 全部子孙分类」后 IN 过滤；{@code limit} 收敛到 [1,50]。
     */
    public List<HotArticleVO> hotArticles(Long categoryId, int limit) {
        int safeLimit = clamp(limit, 1, 50);
        List<Long> categoryIds = categoryId != null ? categoryService.subtreeIds(categoryId) : null;
        List<KbArticle> rows = articleMapper.selectList(Wrappers.<KbArticle>lambdaQuery()
                .eq(KbArticle::getStatus, ONLINE)
                .in(categoryIds != null, KbArticle::getCategoryId, categoryIds)
                .orderByDesc(KbArticle::getViewCount)
                .orderByDesc(KbArticle::getId)
                .last("limit " + safeLimit));
        if (rows.isEmpty()) {
            return Collections.emptyList();
        }
        Map<Long, String> categoryNames = categoryNameMap(rows.stream()
                .map(KbArticle::getCategoryId).filter(Objects::nonNull).collect(Collectors.toSet()));
        return rows.stream().map(a -> {
            HotArticleVO item = new HotArticleVO();
            item.setId(a.getId());
            item.setTitle(a.getTitle());
            item.setCategoryId(a.getCategoryId());
            item.setCategoryName(categoryNames.get(a.getCategoryId()));
            item.setKnowledgeType(a.getKnowledgeType());
            item.setStatus(a.getStatus());
            item.setViewCount(a.getViewCount());
            item.setLikeCount(a.getLikeCount());
            item.setFavoriteCount(a.getFavoriteCount());
            item.setCommentCount(a.getCommentCount());
            return item;
        }).toList();
    }

    // ---------------------------------------------------------------- 热门搜索词

    /**
     * 热门搜索词：kb_search_log 按 keyword 聚合检索次数倒序。
     * {@code days > 0} 时只统计最近 days 天（自然日，含今日）；{@code days <= 0} 统计全部历史。
     */
    public List<HotKeywordVO> hotKeywords(int days, int limit) {
        int safeLimit = clamp(limit, 1, 50);
        LocalDateTime since = days > 0 ? LocalDate.now().minusDays(days - 1L).atStartOfDay() : null;
        List<HotKeywordVO> list = statisticsMapper.hotKeywords(since, safeLimit);
        return list == null ? Collections.emptyList() : list;
    }

    // ---------------------------------------------------------------- 私有辅助

    private Long countByStatus(String status) {
        return articleMapper.selectCount(Wrappers.<KbArticle>lambdaQuery()
                .eq(KbArticle::getStatus, status));
    }

    private Map<Long, String> categoryNameMap(Set<Long> ids) {
        if (ids == null || ids.isEmpty()) {
            return Collections.emptyMap();
        }
        return categoryMapper.selectBatchIds(ids).stream()
                .collect(Collectors.toMap(KbCategory::getId, KbCategory::getName, (a, b) -> a));
    }

    private static int clamp(int value, int min, int max) {
        return Math.max(min, Math.min(max, value));
    }
}
