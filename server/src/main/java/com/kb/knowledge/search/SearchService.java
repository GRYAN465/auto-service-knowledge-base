package com.kb.knowledge.search;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.PageResult;
import com.kb.common.TextUtils;
import com.kb.knowledge.article.dto.TagBriefVO;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.entity.KbArticleTag;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.article.mapper.KbArticleTagMapper;
import com.kb.knowledge.category.CategoryService;
import com.kb.knowledge.search.dto.SearchArticleVO;
import com.kb.knowledge.search.entity.KbSearchLog;
import com.kb.knowledge.search.mapper.KbSearchLogMapper;
import com.kb.knowledge.tag.entity.KbTag;
import com.kb.knowledge.tag.mapper.KbTagMapper;
import com.kb.security.SecurityUtils;
import com.kb.system.user.entity.SysUser;
import com.kb.system.user.mapper.SysUserMapper;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.time.LocalDateTime;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * 知识门户检索（模块 5）：基于 MySQL {@code ft_art_content} ngram 全文索引做关键词检索，
 * 叠加分类/标签/作者筛选，仅返回 ONLINE 知识，并写 kb_search_log 埋点。
 *
 * <p>社区信息流模式支持 {@code offset} 分页 + {@code sortBy} 排序。
 */
@Service
public class SearchService {

    private static final String ONLINE = "ONLINE";
    private static final String SORT_UPDATE = "UPDATE_TIME";
    private static final String SORT_VIEW = "VIEW_COUNT";

    private final KbArticleMapper articleMapper;
    private final KbArticleTagMapper articleTagMapper;
    private final KbSearchLogMapper searchLogMapper;
    private final CategoryService categoryService;
    private final KbTagMapper tagMapper;
    private final SysUserMapper userMapper;

    public SearchService(KbArticleMapper articleMapper, KbArticleTagMapper articleTagMapper,
                         KbSearchLogMapper searchLogMapper, CategoryService categoryService,
                         KbTagMapper tagMapper, SysUserMapper userMapper) {
        this.articleMapper = articleMapper;
        this.articleTagMapper = articleTagMapper;
        this.searchLogMapper = searchLogMapper;
        this.categoryService = categoryService;
        this.tagMapper = tagMapper;
        this.userMapper = userMapper;
    }

    /** 经典 page 分页（管理/兼容）。 */
    public PageResult<SearchArticleVO> search(long page, long pageSize, String keyword, Long categoryId,
                                              Long tagId, Long authorId, String sortBy, String clientIp) {
        int offset = (int) ((page - 1) * pageSize);
        return searchByOffset(offset, pageSize, keyword, categoryId, tagId, authorId, sortBy, clientIp);
    }

    /** 社区信息流 offset 分页：传 offset 时忽略 page 语义，page 字段回写 offset/pageSize+1。 */
    public PageResult<SearchArticleVO> searchByOffset(int offset, long pageSize, String keyword, Long categoryId,
                                                      Long tagId, Long authorId, String sortBy, String clientIp) {
        List<Long> articleIdsByTag = resolveTagArticleIds(tagId);
        if (articleIdsByTag != null && articleIdsByTag.isEmpty()) {
            writeSearchLog(keyword, 0, clientIp);
            long page = pageSize > 0 ? offset / pageSize + 1 : 1;
            return new PageResult<>(0, page, pageSize, Collections.emptyList());
        }

        List<Long> categoryIds = categoryId != null ? categoryService.subtreeIds(categoryId) : null;
        var wrapper = buildWrapper(keyword, categoryIds, articleIdsByTag, authorId, sortBy);

        Long total = articleMapper.selectCount(wrapper);
        List<KbArticle> rows = articleMapper.selectList(wrapper
                .last("LIMIT " + Math.max(0, offset) + ", " + pageSize));
        List<SearchArticleVO> list = toSearchItems(rows);
        writeSearchLog(keyword, total == null ? 0 : total.intValue(), clientIp);
        long page = pageSize > 0 ? offset / pageSize + 1 : 1;
        return new PageResult<>(total == null ? 0 : total, page, pageSize, list);
    }

    /** 供 ArticleService / InteractionService 复用：实体列表 → 信息流 VO。 */
    public List<SearchArticleVO> toSearchItems(List<KbArticle> rows) {
        if (rows == null || rows.isEmpty()) {
            return Collections.emptyList();
        }
        Map<Long, String> authorNames = userNameMap(rows.stream()
                .map(KbArticle::getAuthorId).filter(Objects::nonNull).collect(Collectors.toSet()));
        Map<Long, List<TagBriefVO>> tags = tagsByArticle(rows.stream()
                .map(KbArticle::getId).collect(Collectors.toSet()));

        return rows.stream().map(a -> {
            SearchArticleVO vo = new SearchArticleVO();
            vo.setId(a.getId());
            vo.setTitle(a.getTitle());
            vo.setSummary(a.getSummary());
            vo.setContentPreview(TextUtils.contentPreview(a.getContent(), a.getSummary(), 20));
            vo.setViewCount(a.getViewCount());
            vo.setLikeCount(a.getLikeCount());
            vo.setCommentCount(a.getCommentCount());
            vo.setAuthorId(a.getAuthorId());
            vo.setAuthorName(authorNames.get(a.getAuthorId()));
            vo.setUpdateTime(a.getUpdateTime());
            vo.setTags(tags.getOrDefault(a.getId(), Collections.emptyList()));
            return vo;
        }).toList();
    }

    private com.baomidou.mybatisplus.core.conditions.query.LambdaQueryWrapper<KbArticle> buildWrapper(
            String keyword, List<Long> categoryIds, List<Long> articleIdsByTag, Long authorId, String sortBy) {
        boolean hasKeyword = StringUtils.hasText(keyword);
        var wrapper = Wrappers.<KbArticle>lambdaQuery()
                .eq(KbArticle::getStatus, ONLINE)
                .in(categoryIds != null, KbArticle::getCategoryId, categoryIds)
                .in(articleIdsByTag != null, KbArticle::getId, articleIdsByTag)
                .eq(authorId != null, KbArticle::getAuthorId, authorId);
        if (hasKeyword) {
            wrapper.apply("MATCH(title,summary,content) AGAINST ({0} IN NATURAL LANGUAGE MODE)", keyword.trim());
        } else if (SORT_VIEW.equalsIgnoreCase(sortBy)) {
            wrapper.orderByDesc(KbArticle::getViewCount).orderByDesc(KbArticle::getUpdateTime);
        } else {
            wrapper.orderByDesc(KbArticle::getUpdateTime).orderByDesc(KbArticle::getId);
        }
        return wrapper;
    }

    private List<Long> resolveTagArticleIds(Long tagId) {
        if (tagId == null) {
            return null;
        }
        return articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                        .eq(KbArticleTag::getTagId, tagId))
                .stream().map(KbArticleTag::getArticleId).distinct().toList();
    }

    private Map<Long, String> userNameMap(Set<Long> ids) {
        if (ids == null || ids.isEmpty()) {
            return Collections.emptyMap();
        }
        return userMapper.selectBatchIds(ids).stream().collect(Collectors.toMap(SysUser::getId,
                u -> StringUtils.hasText(u.getRealName()) ? u.getRealName() : u.getUsername(), (a, b) -> a));
    }

    private Map<Long, List<TagBriefVO>> tagsByArticle(Set<Long> articleIds) {
        if (articleIds == null || articleIds.isEmpty()) {
            return Collections.emptyMap();
        }
        List<KbArticleTag> relations = articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                .in(KbArticleTag::getArticleId, articleIds));
        if (relations.isEmpty()) {
            return Collections.emptyMap();
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

    private void writeSearchLog(String keyword, int resultCount, String clientIp) {
        KbSearchLog log = new KbSearchLog();
        log.setUserId(SecurityUtils.getUserIdOrNull());
        log.setKeyword(StringUtils.hasText(keyword) ? keyword.trim() : null);
        log.setResultCount(resultCount);
        log.setClientIp(clientIp);
        log.setCreateTime(LocalDateTime.now());
        searchLogMapper.insert(log);
    }
}
