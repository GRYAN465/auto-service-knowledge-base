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
import com.kb.statistics.dto.AuditOverviewVO;
import com.kb.statistics.dto.CategoryRankVO;
import com.kb.statistics.dto.DateCountVO;
import com.kb.statistics.dto.HotArticleVO;
import com.kb.statistics.dto.HotKeywordVO;
import com.kb.statistics.dto.NameCountVO;
import com.kb.statistics.dto.OverviewVO;
import com.kb.statistics.dto.QaOverviewVO;
import com.kb.statistics.dto.TrendVO;
import com.kb.statistics.mapper.StatisticsMapper;
import org.springframework.stereotype.Service;

import java.time.LocalDate;
import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
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

    // ---------------------------------------------------------------- 日趋势

    public TrendVO trend(int days) {
        int safeDays = clamp(days, 7, 90);
        LocalDateTime since = LocalDate.now().minusDays(safeDays - 1L).atStartOfDay();
        TrendVO vo = new TrendVO();
        vo.setDays(safeDays);
        vo.setGranularity("day");
        vo.setViews(fillDailyGaps(statisticsMapper.dailyViews(since), safeDays));
        vo.setSearches(fillDailyGaps(statisticsMapper.dailySearches(since), safeDays));
        vo.setNewArticles(fillDailyGaps(statisticsMapper.dailyNewArticles(since), safeDays));
        vo.setOnlineArticles(fillDailyGaps(statisticsMapper.dailyOnlineArticles(since), safeDays));
        return vo;
    }

    public TrendVO trendMonthly(int year) {
        int safeYear = year > 0 ? year : LocalDate.now().getYear();
        LocalDateTime start = LocalDate.of(safeYear, 1, 1).atStartOfDay();
        LocalDateTime end = LocalDate.of(safeYear + 1, 1, 1).atStartOfDay();
        TrendVO vo = new TrendVO();
        vo.setDays(12);
        vo.setYear(safeYear);
        vo.setGranularity("month");
        vo.setViews(fillMonthlyGaps(safeYear, statisticsMapper.monthlyViews(start, end)));
        vo.setSearches(fillMonthlyGaps(safeYear, statisticsMapper.monthlySearches(start, end)));
        vo.setNewArticles(fillMonthlyGaps(safeYear, statisticsMapper.monthlyNewArticles(start, end)));
        vo.setOnlineArticles(fillMonthlyGaps(safeYear, statisticsMapper.monthlyOnlineArticles(start, end)));
        return vo;
    }

    // ---------------------------------------------------------------- 分类排行

    public List<CategoryRankVO> categoryRank(int limit) {
        int safeLimit = clamp(limit, 1, 20);
        List<CategoryRankVO> list = statisticsMapper.categoryRank(safeLimit);
        return list == null ? Collections.emptyList() : list;
    }

    // ---------------------------------------------------------------- 审核运营

    public AuditOverviewVO auditOverview() {
        AuditOverviewVO vo = new AuditOverviewVO();
        vo.setPendingAudit(countByStatus(PENDING_AUDIT));
        List<NameCountVO> dist = statisticsMapper.auditResultDist();
        vo.setResultDist(dist == null ? Collections.emptyList() : dist);
        long pass = 0;
        long reject = 0;
        if (dist != null) {
            for (NameCountVO item : dist) {
                if ("PASS".equals(item.getName())) {
                    pass = item.getCount() == null ? 0 : item.getCount();
                } else if ("REJECT".equals(item.getName())) {
                    reject = item.getCount() == null ? 0 : item.getCount();
                }
            }
        }
        vo.setPassCount(pass);
        vo.setRejectCount(reject);
        long total = pass + reject;
        vo.setPassRate(total > 0 ? (double) pass / total : null);
        return vo;
    }

    // ---------------------------------------------------------------- 智能问答运营

    public QaOverviewVO qaOverview(int days) {
        int safeDays = clamp(days, 7, 90);
        LocalDateTime since = LocalDate.now().minusDays(safeDays - 1L).atStartOfDay();
        QaOverviewVO vo = new QaOverviewVO();
        vo.setDays(safeDays);
        Long sessions = statisticsMapper.qaSessionCount(since);
        Long messages = statisticsMapper.qaMessageCount(since);
        Long helpful = statisticsMapper.qaHelpfulCount(since);
        Long unhelpful = statisticsMapper.qaUnhelpfulCount(since);
        vo.setSessionTotal(sessions == null ? 0L : sessions);
        vo.setMessageTotal(messages == null ? 0L : messages);
        vo.setHelpfulCount(helpful == null ? 0L : helpful);
        vo.setUnhelpfulCount(unhelpful == null ? 0L : unhelpful);
        long fbTotal = vo.getHelpfulCount() + vo.getUnhelpfulCount();
        vo.setHelpfulRate(fbTotal > 0 ? (double) vo.getHelpfulCount() / fbTotal : null);
        vo.setSessionTrend(fillDailyGaps(statisticsMapper.dailyQaSessions(since), safeDays));
        return vo;
    }

    // ---------------------------------------------------------------- 私有辅助

    private List<DateCountVO> fillDailyGaps(List<DateCountVO> rows, int days) {
        Map<String, Long> map = new HashMap<>();
        if (rows != null) {
            for (DateCountVO row : rows) {
                if (row.getDate() != null) {
                    map.put(row.getDate(), row.getCount() == null ? 0L : row.getCount());
                }
            }
        }
        List<DateCountVO> out = new ArrayList<>(days);
        LocalDate start = LocalDate.now().minusDays(days - 1L);
        for (int i = 0; i < days; i++) {
            LocalDate d = start.plusDays(i);
            DateCountVO item = new DateCountVO();
            item.setDate(d.toString());
            item.setCount(map.getOrDefault(d.toString(), 0L));
            out.add(item);
        }
        return out;
    }

    private List<DateCountVO> fillMonthlyGaps(int year, List<DateCountVO> rows) {
        Map<String, Long> map = new HashMap<>();
        if (rows != null) {
            for (DateCountVO row : rows) {
                if (row.getDate() != null) {
                    map.put(row.getDate(), row.getCount() == null ? 0L : row.getCount());
                }
            }
        }
        List<DateCountVO> out = new ArrayList<>(12);
        for (int m = 1; m <= 12; m++) {
            String key = String.format("%04d-%02d", year, m);
            DateCountVO item = new DateCountVO();
            item.setDate(key);
            item.setCount(map.getOrDefault(key, 0L));
            out.add(item);
        }
        return out;
    }

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
