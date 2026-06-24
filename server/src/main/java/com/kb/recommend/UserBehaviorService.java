package com.kb.recommend;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.interaction.entity.KbLike;
import com.kb.interaction.mapper.KbLikeMapper;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.entity.KbArticleTag;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.article.mapper.KbArticleTagMapper;
import com.kb.knowledge.search.entity.KbSearchLog;
import com.kb.knowledge.search.mapper.KbSearchLogMapper;
import com.kb.knowledge.tag.entity.KbTag;
import com.kb.knowledge.tag.mapper.KbTagMapper;
import lombok.Data;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Collectors;

/**
 * 聚合首页推荐所需的用户行为：即时/近期搜索、点赞、常用标签 pin。
 */
@Service
public class UserBehaviorService {

    private static final String ONLINE = "ONLINE";
    private static final String ARTICLE = "ARTICLE";

    private final KbSearchLogMapper searchLogMapper;
    private final KbLikeMapper likeMapper;
    private final KbArticleMapper articleMapper;
    private final KbArticleTagMapper articleTagMapper;
    private final KbTagMapper tagMapper;

    @Value("${kb.recommend.immediate-search-hours:24}")
    private int immediateSearchHours;

    @Value("${kb.recommend.recent-search-days:7}")
    private int recentSearchDays;

    @Value("${kb.recommend.immediate-search-limit:3}")
    private int immediateSearchLimit;

    @Value("${kb.recommend.search-limit:10}")
    private int recentSearchLimit;

    @Value("${kb.recommend.like-limit:5}")
    private int likeLimit;

    public UserBehaviorService(KbSearchLogMapper searchLogMapper, KbLikeMapper likeMapper,
                               KbArticleMapper articleMapper, KbArticleTagMapper articleTagMapper,
                               KbTagMapper tagMapper) {
        this.searchLogMapper = searchLogMapper;
        this.likeMapper = likeMapper;
        this.articleMapper = articleMapper;
        this.articleTagMapper = articleTagMapper;
        this.tagMapper = tagMapper;
    }

    public UserBehavior load(Long userId, List<Long> pinnedTagIds, int searchLimit, int likeLimit) {
        UserBehavior behavior = new UserBehavior();
        behavior.setUserId(userId);
        behavior.setPinnedTagIds(pinnedTagIds == null ? List.of() : pinnedTagIds);
        loadSearchKeywords(userId, behavior);
        behavior.setRecentLikes(loadRecentLikes(userId, likeLimit > 0 ? likeLimit : this.likeLimit));
        return behavior;
    }

    public boolean isEmpty(UserBehavior behavior) {
        return behavior.getImmediateKeywords().isEmpty()
                && behavior.getRecentKeywords().isEmpty()
                && behavior.getRecentLikes().isEmpty()
                && behavior.getPinnedTagIds().isEmpty();
    }

    /** 按优先级构建画像分段：即时搜索 > 近期搜索 > 点赞 > 常用标签。 */
    public List<ProfileSegment> buildProfileSegments(UserBehavior behavior, Map<Long, String> tagNameById) {
        List<ProfileSegment> segments = new ArrayList<>();
        if (!behavior.getImmediateKeywords().isEmpty()) {
            segments.add(new ProfileSegment(
                    "最近搜索：" + String.join("；", behavior.getImmediateKeywords()), "immediate"));
        }
        List<String> recentOnly = behavior.getRecentKeywords().stream()
                .filter(kw -> !behavior.getImmediateKeywords().contains(kw))
                .toList();
        if (!recentOnly.isEmpty()) {
            segments.add(new ProfileSegment(
                    "近期搜索：" + String.join("；", recentOnly), "recent"));
        }
        if (!behavior.getRecentLikes().isEmpty()) {
            StringBuilder likeText = new StringBuilder();
            for (LikedArticleBrief like : behavior.getRecentLikes()) {
                likeText.append("最近点赞：《").append(like.getTitle()).append("》");
                if (!like.getTagNames().isEmpty()) {
                    likeText.append("（标签：").append(String.join(",", like.getTagNames())).append("）");
                }
                likeText.append("；");
            }
            segments.add(new ProfileSegment(likeText.toString().trim(), "like"));
        }
        if (behavior.getPinnedTagIds() != null && !behavior.getPinnedTagIds().isEmpty()) {
            List<String> pinNames = new ArrayList<>();
            for (Long tagId : behavior.getPinnedTagIds()) {
                String name = tagNameById.get(tagId);
                if (StringUtils.hasText(name)) {
                    pinNames.add(name);
                }
            }
            if (!pinNames.isEmpty()) {
                segments.add(new ProfileSegment(
                        "常用标签：" + String.join("；", pinNames), "pin"));
            }
        }
        return segments;
    }

    /** 最近行为强度 [0,1]，用于动态提高兴趣分权重。 */
    public double recentBehaviorStrength(UserBehavior behavior) {
        double score = 0;
        score += Math.min(1.0, behavior.getImmediateKeywords().size() / 3.0) * 0.55;
        score += Math.min(1.0, behavior.getRecentKeywords().size() / 5.0) * 0.25;
        score += Math.min(1.0, behavior.getRecentLikes().size() / 3.0) * 0.20;
        return Math.min(1.0, score);
    }

    public List<String> displayKeywords(UserBehavior behavior) {
        LinkedHashSet<String> ordered = new LinkedHashSet<>();
        ordered.addAll(behavior.getImmediateKeywords());
        ordered.addAll(behavior.getRecentKeywords());
        return new ArrayList<>(ordered);
    }

    private void loadSearchKeywords(Long userId, UserBehavior behavior) {
        if (userId == null) {
            behavior.setImmediateKeywords(List.of());
            behavior.setRecentKeywords(List.of());
            return;
        }
        LocalDateTime now = LocalDateTime.now();
        LocalDateTime immediateSince = now.minusHours(Math.max(1, immediateSearchHours));
        LocalDateTime recentSince = now.minusDays(Math.max(1, recentSearchDays));

        int fetchLimit = Math.max(recentSearchLimit, immediateSearchLimit) * 4;
        List<KbSearchLog> rows = searchLogMapper.selectList(Wrappers.<KbSearchLog>lambdaQuery()
                .eq(KbSearchLog::getUserId, userId)
                .isNotNull(KbSearchLog::getKeyword)
                .ne(KbSearchLog::getKeyword, "")
                .ge(KbSearchLog::getCreateTime, recentSince)
                .orderByDesc(KbSearchLog::getCreateTime)
                .last("LIMIT " + fetchLimit));

        LinkedHashSet<String> immediate = new LinkedHashSet<>();
        LinkedHashSet<String> recent = new LinkedHashSet<>();
        for (KbSearchLog row : rows) {
            if (!StringUtils.hasText(row.getKeyword()) || row.getCreateTime() == null) {
                continue;
            }
            String kw = row.getKeyword().trim();
            if (row.getCreateTime().isAfter(immediateSince) || row.getCreateTime().isEqual(immediateSince)) {
                if (immediate.size() < immediateSearchLimit) {
                    immediate.add(kw);
                }
            }
            if (recent.size() < recentSearchLimit) {
                recent.add(kw);
            }
            if (immediate.size() >= immediateSearchLimit && recent.size() >= recentSearchLimit) {
                break;
            }
        }
        behavior.setImmediateKeywords(new ArrayList<>(immediate));
        behavior.setRecentKeywords(new ArrayList<>(recent));
    }

    private List<LikedArticleBrief> loadRecentLikes(Long userId, int limit) {
        if (userId == null) {
            return List.of();
        }
        List<KbLike> likes = likeMapper.selectList(Wrappers.<KbLike>lambdaQuery()
                .eq(KbLike::getUserId, userId)
                .eq(KbLike::getTargetType, ARTICLE)
                .eq(KbLike::getType, 1)
                .orderByDesc(KbLike::getCreateTime)
                .last("LIMIT " + Math.max(1, limit)));
        if (likes.isEmpty()) {
            return List.of();
        }
        List<Long> articleIds = likes.stream().map(KbLike::getTargetId).distinct().toList();
        Map<Long, KbArticle> articles = articleMapper.selectBatchIds(articleIds).stream()
                .filter(a -> ONLINE.equals(a.getStatus()))
                .collect(Collectors.toMap(KbArticle::getId, a -> a, (a, b) -> a));
        Map<Long, List<String>> tagNames = tagNamesByArticle(articleIds);

        List<LikedArticleBrief> result = new ArrayList<>();
        for (KbLike like : likes) {
            KbArticle article = articles.get(like.getTargetId());
            if (article == null) {
                continue;
            }
            LikedArticleBrief brief = new LikedArticleBrief();
            brief.setArticleId(article.getId());
            brief.setTitle(article.getTitle());
            brief.setTagNames(tagNames.getOrDefault(article.getId(), List.of()));
            result.add(brief);
            if (result.size() >= limit) {
                break;
            }
        }
        return result;
    }

    private Map<Long, List<String>> tagNamesByArticle(List<Long> articleIds) {
        if (articleIds == null || articleIds.isEmpty()) {
            return Map.of();
        }
        List<KbArticleTag> relations = articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                .in(KbArticleTag::getArticleId, articleIds));
        if (relations.isEmpty()) {
            return Map.of();
        }
        Map<Long, KbTag> tagById = tagMapper.selectBatchIds(relations.stream()
                        .map(KbArticleTag::getTagId).collect(Collectors.toSet())).stream()
                .collect(Collectors.toMap(KbTag::getId, t -> t, (a, b) -> a));
        return relations.stream().collect(Collectors.groupingBy(KbArticleTag::getArticleId,
                Collectors.mapping(r -> {
                    KbTag tag = tagById.get(r.getTagId());
                    return tag == null ? null : tag.getName();
                }, Collectors.filtering(Objects::nonNull, Collectors.toList()))));
    }

    @Data
    public static class UserBehavior {
        private Long userId;
        private List<Long> pinnedTagIds = List.of();
        /** 近 24h 搜索词（默认最多 3 条）。 */
        private List<String> immediateKeywords = List.of();
        /** 近 7d 搜索词（默认最多 10 条）。 */
        private List<String> recentKeywords = List.of();
        private List<LikedArticleBrief> recentLikes = List.of();
    }

    @Data
    public static class LikedArticleBrief {
        private Long articleId;
        private String title;
        private List<String> tagNames = List.of();
    }
}
