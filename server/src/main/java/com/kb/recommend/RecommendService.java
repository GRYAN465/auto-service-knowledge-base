package com.kb.recommend;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.ai.AiClient;
import com.kb.ai.dto.AiRecommendMatchRequest;
import com.kb.ai.dto.AiRecommendMatchResponse;
import com.kb.knowledge.article.dto.TagBriefVO;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.entity.KbArticleTag;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.article.mapper.KbArticleTagMapper;
import com.kb.knowledge.category.entity.KbCategory;
import com.kb.knowledge.category.mapper.KbCategoryMapper;
import com.kb.knowledge.tag.entity.KbTag;
import com.kb.knowledge.tag.mapper.KbTagMapper;
import com.kb.knowledge.search.SearchService;
import com.kb.recommend.dto.RecommendArticleVO;
import com.kb.recommend.dto.RecommendHomeVO;
import com.kb.recommend.dto.RecommendProfileSummaryVO;
import com.kb.recommend.dto.RecommendTagScoreVO;
import com.kb.security.SecurityUtils;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * 首页个性化推荐：分层行为画像 → ai-service 加权向量匹配 → 多路召回 → 兴趣优先排序。
 */
@Slf4j
@Service
public class RecommendService {

    private static final String ONLINE = "ONLINE";

    private final UserBehaviorService userBehaviorService;
    private final AiClient aiClient;
    private final SearchService searchService;
    private final KbTagMapper tagMapper;
    private final KbArticleMapper articleMapper;
    private final KbArticleTagMapper articleTagMapper;
    private final KbCategoryMapper categoryMapper;

    @Value("${kb.recommend.search-limit:10}")
    private int searchLimit;

    @Value("${kb.recommend.like-limit:5}")
    private int likeLimit;

    @Value("${kb.recommend.top-tags:5}")
    private int topTags;

    @Value("${kb.recommend.candidate-per-tag:50}")
    private int candidatePerTag;

    @Value("${kb.recommend.result-limit:5}")
    private int resultLimit;

    @Value("${kb.recommend.profile-weight-immediate:0.45}")
    private double profileWeightImmediate;

    @Value("${kb.recommend.profile-weight-recent-search:0.25}")
    private double profileWeightRecentSearch;

    @Value("${kb.recommend.profile-weight-recent-like:0.15}")
    private double profileWeightRecentLike;

    @Value("${kb.recommend.profile-weight-pinned-tag:0.15}")
    private double profileWeightPinnedTag;

    @Value("${kb.recommend.weight-interest-base:0.72}")
    private double weightInterestBase;

    @Value("${kb.recommend.weight-interest-boost:0.13}")
    private double weightInterestBoost;

    @Value("${kb.recommend.interest-article-sim:0.35}")
    private double interestArticleSim;

    @Value("${kb.recommend.interest-tag-sim:0.30}")
    private double interestTagSim;

    @Value("${kb.recommend.interest-keyword-hit:0.25}")
    private double interestKeywordHit;

    @Value("${kb.recommend.interest-like-tag:0.10}")
    private double interestLikeTag;

    @Value("${kb.recommend.search-recall-limit:20}")
    private int searchRecallLimit;

    @Value("${kb.recommend.max-embed-articles:120}")
    private int maxEmbedArticles;

    @Value("${kb.recommend.hot-view-weight:0.45}")
    private double hotViewWeight;

    @Value("${kb.recommend.hot-like-weight:0.35}")
    private double hotLikeWeight;

    @Value("${kb.recommend.hot-comment-weight:0.20}")
    private double hotCommentWeight;

    public RecommendService(UserBehaviorService userBehaviorService, AiClient aiClient,
                            SearchService searchService, KbTagMapper tagMapper,
                            KbArticleMapper articleMapper, KbArticleTagMapper articleTagMapper,
                            KbCategoryMapper categoryMapper) {
        this.userBehaviorService = userBehaviorService;
        this.aiClient = aiClient;
        this.searchService = searchService;
        this.tagMapper = tagMapper;
        this.articleMapper = articleMapper;
        this.articleTagMapper = articleTagMapper;
        this.categoryMapper = categoryMapper;
    }

    public RecommendHomeVO homeRecommend(List<Long> pinnedTagIds, Integer limit) {
        int safeLimit = clamp(limit == null ? resultLimit : limit, 1, 20);
        Long userId = SecurityUtils.getUserIdOrNull();

        List<KbTag> allTags = tagMapper.selectList(Wrappers.<KbTag>lambdaQuery()
                .orderByAsc(KbTag::getSort)
                .orderByAsc(KbTag::getId));
        Map<Long, String> tagNameById = allTags.stream()
                .collect(Collectors.toMap(KbTag::getId, KbTag::getName, (a, b) -> a));

        UserBehaviorService.UserBehavior behavior =
                userBehaviorService.load(userId, pinnedTagIds, searchLimit, likeLimit);

        RecommendProfileSummaryVO summary = new RecommendProfileSummaryVO();
        summary.setKeywords(userBehaviorService.displayKeywords(behavior));

        if (userBehaviorService.isEmpty(behavior)) {
            RecommendHomeVO vo = fallbackHot(safeLimit, allTags, summary);
            summary.setSource("fallback");
            vo.setProfileSummary(summary);
            vo.setFallback(true);
            return vo;
        }

        List<ProfileSegment> profileSegments =
                userBehaviorService.buildProfileSegments(behavior, tagNameById);
        if (profileSegments.isEmpty()) {
            RecommendHomeVO vo = fallbackHot(safeLimit, allTags, summary);
            summary.setSource("fallback");
            vo.setProfileSummary(summary);
            vo.setFallback(true);
            return vo;
        }

        Set<Long> excludeArticleIds = behavior.getRecentLikes().stream()
                .map(UserBehaviorService.LikedArticleBrief::getArticleId)
                .collect(Collectors.toSet());

        AiRecommendMatchResponse matchResponse;
        try {
            matchResponse = aiClient.recommendMatch(
                    buildMatchRequest(profileSegments, allTags, behavior));
        } catch (Exception e) {
            log.warn("推荐向量匹配失败，降级热门：{} — 请确认 ai-service(8000) 已启动且 /ai/recommend/match 可访问",
                    e.toString());
            RecommendHomeVO vo = fallbackHot(safeLimit, allTags, summary);
            summary.setSource("fallback");
            vo.setProfileSummary(summary);
            vo.setFallback(true);
            return vo;
        }

        List<AiRecommendMatchResponse.AiRecommendTagScore> matchedTags =
                matchResponse.getTopTags() == null ? List.of() : matchResponse.getTopTags();
        summary.setTopTags(matchedTags.stream().map(t -> {
            RecommendTagScoreVO item = new RecommendTagScoreVO();
            item.setId(t.getId());
            item.setName(t.getName());
            item.setScore(t.getScore());
            return item;
        }).toList());
        summary.setSource("vector");

        Map<Long, Double> tagSimById = matchedTags.stream()
                .filter(t -> t.getId() != null && t.getScore() != null)
                .collect(Collectors.toMap(AiRecommendMatchResponse.AiRecommendTagScore::getId,
                        AiRecommendMatchResponse.AiRecommendTagScore::getScore, (a, b) -> a));

        Map<Long, Double> articleSimById = new HashMap<>();
        if (matchResponse.getTopArticles() != null) {
            for (AiRecommendMatchResponse.AiRecommendArticleScore a : matchResponse.getTopArticles()) {
                if (a.getId() != null && a.getScore() != null) {
                    articleSimById.put(a.getId(), a.getScore());
                }
            }
        }

        List<Long> interestTagIds = resolveInterestTagIds(matchedTags);

        Map<Long, KbArticle> candidates =
                collectCandidates(interestTagIds, behavior, excludeArticleIds, tagNameById);
        if (candidates.isEmpty()) {
            RecommendHomeVO vo = fallbackHot(safeLimit, allTags, summary);
            vo.setFallback(true);
            vo.setProfileSummary(summary);
            return vo;
        }

        Map<Long, List<TagBriefVO>> tagsByArticle = tagsByArticle(candidates.keySet());
        Map<Long, Double> rawHot = new HashMap<>();
        Map<Long, Double> interestRaw = new HashMap<>();
        for (KbArticle article : candidates.values()) {
            long view = article.getViewCount() == null ? 0 : article.getViewCount();
            long like = article.getLikeCount() == null ? 0 : article.getLikeCount();
            long comment = article.getCommentCount() == null ? 0 : article.getCommentCount();
            rawHot.put(article.getId(), HotScoreCalculator.rawScore(
                    view, like, comment, hotViewWeight, hotLikeWeight, hotCommentWeight));

            double tagSim = tagsByArticle.getOrDefault(article.getId(), List.of()).stream()
                    .map(TagBriefVO::getId)
                    .map(tagSimById::get)
                    .filter(Objects::nonNull)
                    .mapToDouble(Double::doubleValue)
                    .max().orElse(0);
            double artSim = articleSimById.getOrDefault(article.getId(), 0.0);
            double kwHit = keywordHitScore(article, behavior);
            double likeTag = likeTagBoost(article, tagsByArticle, behavior);
            interestRaw.put(article.getId(),
                    Math.min(1.0, interestArticleSim * artSim
                            + interestTagSim * tagSim
                            + interestKeywordHit * kwHit
                            + interestLikeTag * likeTag));
        }

        Map<Long, Double> hotNorm = HotScoreCalculator.normalize(rawHot);
        List<Double> interestPool = new ArrayList<>(interestRaw.values());
        double wInterest = resolveInterestWeight(behavior);
        double wHot = 1.0 - wInterest;
        List<ScoredArticle> ranked = new ArrayList<>();
        for (KbArticle article : candidates.values()) {
            double interestNorm = HotScoreCalculator.normalizeValue(
                    interestRaw.getOrDefault(article.getId(), 0.0), interestPool);
            double hot = hotNorm.getOrDefault(article.getId(), 0.5);
            double finalScore = wInterest * interestNorm + wHot * hot;
            ranked.add(new ScoredArticle(article, finalScore, interestNorm, hot, tagsByArticle));
        }
        ranked.sort(Comparator.comparingDouble(ScoredArticle::finalScore).reversed());

        List<RecommendArticleVO> items = pickDiverse(ranked, tagSimById, tagNameById, safeLimit);
        RecommendHomeVO vo = new RecommendHomeVO();
        vo.setItems(items);
        vo.setProfileSummary(summary);
        vo.setFallback(false);
        return vo;
    }

    private AiRecommendMatchRequest buildMatchRequest(List<ProfileSegment> profileSegments,
                                                      List<KbTag> allTags,
                                                      UserBehaviorService.UserBehavior behavior) {
        List<AiRecommendMatchRequest.AiRecommendTagItem> tagItems = allTags.stream()
                .map(t -> new AiRecommendMatchRequest.AiRecommendTagItem(t.getId(), t.getName()))
                .toList();

        LinkedHashSet<Long> embedIds = new LinkedHashSet<>();
        for (UserBehaviorService.LikedArticleBrief like : behavior.getRecentLikes()) {
            embedIds.add(like.getArticleId());
        }
        for (String kw : behavior.getImmediateKeywords()) {
            embedIds.addAll(searchService.recallArticleIdsByKeyword(kw, 15));
        }
        for (String kw : behavior.getRecentKeywords().stream().limit(3).toList()) {
            embedIds.addAll(searchService.recallArticleIdsByKeyword(kw, 10));
        }
        if (behavior.getPinnedTagIds() != null) {
            for (Long tagId : behavior.getPinnedTagIds()) {
                articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                                .eq(KbArticleTag::getTagId, tagId)
                                .last("LIMIT 20"))
                        .forEach(r -> embedIds.add(r.getArticleId()));
            }
        }
        List<KbArticle> hotSeed = articleMapper.selectList(Wrappers.<KbArticle>lambdaQuery()
                .eq(KbArticle::getStatus, ONLINE)
                .orderByDesc(KbArticle::getViewCount)
                .orderByDesc(KbArticle::getId)
                .last("LIMIT 80"));
        hotSeed.forEach(a -> embedIds.add(a.getId()));

        List<Long> limitedIds = embedIds.stream().limit(maxEmbedArticles).toList();
        Map<Long, KbArticle> embedArticles = articleMapper.selectBatchIds(limitedIds).stream()
                .filter(a -> ONLINE.equals(a.getStatus()))
                .collect(Collectors.toMap(KbArticle::getId, a -> a, (a, b) -> a));
        Map<Long, List<TagBriefVO>> tagsByArticle = tagsByArticle(embedArticles.keySet());

        List<AiRecommendMatchRequest.AiRecommendArticleItem> articleItems = new ArrayList<>();
        for (Long id : limitedIds) {
            KbArticle a = embedArticles.get(id);
            if (a == null) {
                continue;
            }
            String tagPart = tagsByArticle.getOrDefault(id, List.of()).stream()
                    .map(TagBriefVO::getName)
                    .collect(Collectors.joining(","));
            String text = a.getTitle();
            if (StringUtils.hasText(a.getSummary())) {
                text += " " + a.getSummary();
            }
            if (StringUtils.hasText(tagPart)) {
                text += " 标签：" + tagPart;
            }
            articleItems.add(new AiRecommendMatchRequest.AiRecommendArticleItem(id, text));
        }

        return new AiRecommendMatchRequest(
                "", toAiProfileSegments(profileSegments), tagItems, articleItems, topTags, 30);
    }

    private List<Long> resolveInterestTagIds(List<AiRecommendMatchResponse.AiRecommendTagScore> matchedTags) {
        return matchedTags.stream()
                .map(AiRecommendMatchResponse.AiRecommendTagScore::getId)
                .filter(Objects::nonNull)
                .limit(topTags)
                .toList();
    }

    private Map<Long, KbArticle> collectCandidates(List<Long> interestTagIds,
                                                   UserBehaviorService.UserBehavior behavior,
                                                   Set<Long> excludeArticleIds,
                                                   Map<Long, String> tagNameById) {
        LinkedHashSet<Long> ids = new LinkedHashSet<>();
        for (Long tagId : interestTagIds) {
            articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                            .eq(KbArticleTag::getTagId, tagId)
                            .last("LIMIT " + candidatePerTag))
                    .forEach(r -> ids.add(r.getArticleId()));
        }
        for (String kw : behavior.getImmediateKeywords()) {
            ids.addAll(searchService.recallArticleIdsByKeyword(kw, searchRecallLimit));
        }
        for (String kw : behavior.getRecentKeywords().stream().limit(3).toList()) {
            ids.addAll(searchService.recallArticleIdsByKeyword(kw, searchRecallLimit / 2));
        }
        Set<Long> likedTagIds = tagIdsFromLikedArticles(behavior, tagNameById);
        for (Long tagId : likedTagIds) {
            articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                            .eq(KbArticleTag::getTagId, tagId)
                            .last("LIMIT " + Math.max(10, candidatePerTag / 2)))
                    .forEach(r -> ids.add(r.getArticleId()));
        }
        if (ids.isEmpty()) {
            articleMapper.selectList(Wrappers.<KbArticle>lambdaQuery()
                            .eq(KbArticle::getStatus, ONLINE)
                            .orderByDesc(KbArticle::getUpdateTime)
                            .last("LIMIT 100"))
                    .forEach(a -> ids.add(a.getId()));
        }
        ids.removeAll(excludeArticleIds);
        if (ids.isEmpty()) {
            return Map.of();
        }
        return articleMapper.selectBatchIds(ids).stream()
                .filter(a -> ONLINE.equals(a.getStatus()))
                .collect(Collectors.toMap(KbArticle::getId, a -> a, (a, b) -> a, LinkedHashMap::new));
    }

    private List<AiRecommendMatchRequest.AiProfileSegment> toAiProfileSegments(
            List<ProfileSegment> segments) {
        List<AiRecommendMatchRequest.AiProfileSegment> out = new ArrayList<>();
        for (ProfileSegment seg : segments) {
            double weight = profileWeightForKind(seg.kind());
            if (weight > 0 && StringUtils.hasText(seg.text())) {
                out.add(new AiRecommendMatchRequest.AiProfileSegment(seg.text(), weight));
            }
        }
        normalizeProfileWeights(out);
        return out;
    }

    private double profileWeightForKind(String kind) {
        return switch (kind) {
            case "immediate" -> profileWeightImmediate;
            case "recent" -> profileWeightRecentSearch;
            case "like" -> profileWeightRecentLike;
            case "pin" -> profileWeightPinnedTag;
            default -> 0;
        };
    }

    private void normalizeProfileWeights(List<AiRecommendMatchRequest.AiProfileSegment> segments) {
        double sum = segments.stream()
                .mapToDouble(AiRecommendMatchRequest.AiProfileSegment::getWeight)
                .sum();
        if (sum <= 0) {
            return;
        }
        if (Math.abs(sum - 1.0) > 0.001) {
            segments.forEach(s -> s.setWeight(s.getWeight() / sum));
        }
    }

    private double resolveInterestWeight(UserBehaviorService.UserBehavior behavior) {
        double strength = userBehaviorService.recentBehaviorStrength(behavior);
        return Math.min(0.85, weightInterestBase + weightInterestBoost * strength);
    }

    private double keywordHitScore(KbArticle article, UserBehaviorService.UserBehavior behavior) {
        String hay = ((article.getTitle() == null ? "" : article.getTitle()) + " "
                + (article.getSummary() == null ? "" : article.getSummary())).toLowerCase();
        double score = 0;
        List<String> immediate = behavior.getImmediateKeywords();
        for (int i = 0; i < immediate.size(); i++) {
            String kw = immediate.get(i).trim().toLowerCase();
            if (kw.length() >= 2 && hay.contains(kw)) {
                score = Math.max(score, 1.0 - i * 0.12);
            }
        }
        for (String kw : behavior.getRecentKeywords()) {
            String k = kw.trim().toLowerCase();
            if (k.length() >= 2 && hay.contains(k)) {
                score = Math.max(score, 0.65);
            }
        }
        return score;
    }

    private double likeTagBoost(KbArticle article, Map<Long, List<TagBriefVO>> tagsByArticle,
                                UserBehaviorService.UserBehavior behavior) {
        Set<String> likedTags = behavior.getRecentLikes().stream()
                .flatMap(l -> l.getTagNames().stream())
                .collect(Collectors.toSet());
        if (likedTags.isEmpty()) {
            return 0;
        }
        return tagsByArticle.getOrDefault(article.getId(), List.of()).stream()
                .map(TagBriefVO::getName)
                .anyMatch(likedTags::contains) ? 1.0 : 0.0;
    }

    private Set<Long> tagIdsFromLikedArticles(UserBehaviorService.UserBehavior behavior,
                                              Map<Long, String> tagNameById) {
        Map<String, Long> nameToId = tagNameById.entrySet().stream()
                .collect(Collectors.toMap(Map.Entry::getValue, Map.Entry::getKey, (a, b) -> a));
        Set<Long> ids = new LinkedHashSet<>();
        for (UserBehaviorService.LikedArticleBrief like : behavior.getRecentLikes()) {
            for (String name : like.getTagNames()) {
                Long id = nameToId.get(name);
                if (id != null) {
                    ids.add(id);
                }
            }
        }
        return ids;
    }

    private RecommendHomeVO fallbackHot(int limit, List<KbTag> allTags, RecommendProfileSummaryVO summary) {
        List<KbArticle> rows = articleMapper.selectList(Wrappers.<KbArticle>lambdaQuery()
                .eq(KbArticle::getStatus, ONLINE)
                .orderByDesc(KbArticle::getViewCount)
                .orderByDesc(KbArticle::getId)
                .last("LIMIT " + Math.max(limit * 3, 20)));
        Map<Long, Double> rawHot = new HashMap<>();
        for (KbArticle a : rows) {
            long view = a.getViewCount() == null ? 0 : a.getViewCount();
            long like = a.getLikeCount() == null ? 0 : a.getLikeCount();
            long comment = a.getCommentCount() == null ? 0 : a.getCommentCount();
            rawHot.put(a.getId(), HotScoreCalculator.rawScore(
                    view, like, comment, hotViewWeight, hotLikeWeight, hotCommentWeight));
        }
        Map<Long, Double> hotNorm = HotScoreCalculator.normalize(rawHot);
        Map<Long, List<TagBriefVO>> tagsByArticle = tagsByArticle(rows.stream()
                .map(KbArticle::getId).collect(Collectors.toSet()));
        Map<Long, String> categoryNames = categoryNameMap(rows.stream()
                .map(KbArticle::getCategoryId).filter(Objects::nonNull).collect(Collectors.toSet()));

        List<ScoredArticle> ranked = rows.stream()
                .map(a -> new ScoredArticle(a, hotNorm.getOrDefault(a.getId(), 0.5),
                        0.0, hotNorm.getOrDefault(a.getId(), 0.5), tagsByArticle))
                .sorted(Comparator.comparingDouble(ScoredArticle::finalScore).reversed())
                .toList();

        List<RecommendArticleVO> items = ranked.stream()
                .limit(limit)
                .map(s -> toVo(s, Map.of(), Map.of(), categoryNames))
                .toList();

        if (summary.getTopTags() == null || summary.getTopTags().isEmpty()) {
            summary.setTopTags(allTags.stream().limit(4).map(t -> {
                RecommendTagScoreVO vo = new RecommendTagScoreVO();
                vo.setId(t.getId());
                vo.setName(t.getName());
                vo.setScore(0.0);
                return vo;
            }).toList());
        }

        RecommendHomeVO vo = new RecommendHomeVO();
        vo.setItems(items);
        vo.setProfileSummary(summary);
        vo.setFallback(true);
        return vo;
    }

    private List<RecommendArticleVO> pickDiverse(List<ScoredArticle> ranked,
                                                 Map<Long, Double> tagSimById,
                                                 Map<Long, String> tagNameById,
                                                 int limit) {
        Map<Long, String> categoryNames = categoryNameMap(ranked.stream()
                .map(s -> s.article.getCategoryId())
                .filter(Objects::nonNull)
                .collect(Collectors.toSet()));

        List<RecommendArticleVO> result = new ArrayList<>();
        Map<Long, Integer> tagPickCount = new HashMap<>();
        for (ScoredArticle scored : ranked) {
            if (result.size() >= limit) {
                break;
            }
            Long primaryTag = scored.tagsByArticle.getOrDefault(scored.article.getId(), List.of()).stream()
                    .map(TagBriefVO::getId)
                    .filter(tagSimById::containsKey)
                    .max(Comparator.comparingDouble(id -> tagSimById.getOrDefault(id, 0.0)))
                    .orElse(null);
            if (primaryTag != null && tagPickCount.getOrDefault(primaryTag, 0) >= 2) {
                continue;
            }
            result.add(toVo(scored, tagSimById, tagNameById, categoryNames));
            if (primaryTag != null) {
                tagPickCount.merge(primaryTag, 1, Integer::sum);
            }
        }
        if (result.size() < limit) {
            Set<Long> picked = result.stream().map(RecommendArticleVO::getId).collect(Collectors.toSet());
            for (ScoredArticle scored : ranked) {
                if (result.size() >= limit) {
                    break;
                }
                if (picked.add(scored.article.getId())) {
                    result.add(toVo(scored, tagSimById, tagNameById, categoryNames));
                }
            }
        }
        return result;
    }

    private RecommendArticleVO toVo(ScoredArticle scored,
                                    Map<Long, Double> tagSimById,
                                    Map<Long, String> tagNameById,
                                    Map<Long, String> categoryNames) {
        KbArticle a = scored.article;
        RecommendArticleVO vo = new RecommendArticleVO();
        vo.setId(a.getId());
        vo.setTitle(a.getTitle());
        vo.setCategoryName(categoryNames.get(a.getCategoryId()));
        vo.setKnowledgeType(a.getKnowledgeType());
        vo.setViewCount(a.getViewCount());
        vo.setLikeCount(a.getLikeCount());
        vo.setCommentCount(a.getCommentCount());
        vo.setTags(scored.tagsByArticle.getOrDefault(a.getId(), List.of()));
        vo.setMatchScore(round3(scored.interestNorm));
        vo.setHotScore(round3(scored.hotNorm));
        List<String> reasons = scored.tagsByArticle.getOrDefault(a.getId(), List.of()).stream()
                .map(TagBriefVO::getId)
                .filter(tagSimById::containsKey)
                .sorted(Comparator.comparingDouble((Long id) -> tagSimById.getOrDefault(id, 0.0)).reversed())
                .map(tagNameById::get)
                .filter(Objects::nonNull)
                .limit(2)
                .toList();
        vo.setReasonTags(reasons);
        return vo;
    }

    private Map<Long, List<TagBriefVO>> tagsByArticle(Set<Long> articleIds) {
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
                    KbTag t = tagById.get(r.getTagId());
                    return t == null ? null : new TagBriefVO(t.getId(), t.getName(), t.getColor());
                }, Collectors.filtering(Objects::nonNull, Collectors.toList()))));
    }

    private Map<Long, String> categoryNameMap(Set<Long> ids) {
        if (ids == null || ids.isEmpty()) {
            return Map.of();
        }
        return categoryMapper.selectBatchIds(ids).stream()
                .collect(Collectors.toMap(KbCategory::getId, KbCategory::getName, (a, b) -> a));
    }

    private static double round3(double v) {
        return Math.round(v * 1000.0) / 1000.0;
    }

    private static int clamp(int value, int min, int max) {
        return Math.max(min, Math.min(max, value));
    }

    private record ScoredArticle(KbArticle article, double finalScore, double interestNorm, double hotNorm,
                               Map<Long, List<TagBriefVO>> tagsByArticle) {
    }
}
