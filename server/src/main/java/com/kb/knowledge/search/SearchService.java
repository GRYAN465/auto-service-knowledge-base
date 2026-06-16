package com.kb.knowledge.search;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.PageResult;
import com.kb.knowledge.article.ArticleService;
import com.kb.knowledge.article.dto.ArticleListItemVO;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.entity.KbArticleTag;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.article.mapper.KbArticleTagMapper;
import com.kb.knowledge.category.CategoryService;
import com.kb.knowledge.search.entity.KbSearchLog;
import com.kb.knowledge.search.mapper.KbSearchLogMapper;
import com.kb.security.SecurityUtils;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.time.LocalDateTime;
import java.util.Collections;
import java.util.List;

/**
 * 知识门户检索（模块 5）：基于 MySQL {@code ft_art_content} ngram 全文索引做关键词检索，
 * 叠加分类/标签筛选，仅返回 ONLINE 知识，并写 kb_search_log 埋点。
 *
 * <p>富集（categoryName/authorName/tagNames）复用 {@link ArticleService#toListItems}，避免 N+1 重复实现。
 */
@Service
public class SearchService {

    private static final String ONLINE = "ONLINE";

    private final KbArticleMapper articleMapper;
    private final KbArticleTagMapper articleTagMapper;
    private final KbSearchLogMapper searchLogMapper;
    private final ArticleService articleService;
    private final CategoryService categoryService;

    public SearchService(KbArticleMapper articleMapper, KbArticleTagMapper articleTagMapper,
                         KbSearchLogMapper searchLogMapper, ArticleService articleService,
                         CategoryService categoryService) {
        this.articleMapper = articleMapper;
        this.articleTagMapper = articleTagMapper;
        this.searchLogMapper = searchLogMapper;
        this.articleService = articleService;
        this.categoryService = categoryService;
    }

    public PageResult<ArticleListItemVO> search(long page, long pageSize, String keyword, Long categoryId,
                                                Long tagId, String clientIp) {
        // tagId 条件：先取该标签下的 articleId 集合，空集合直接返回空页（同 ArticleService.page）。
        List<Long> articleIdsByTag = null;
        if (tagId != null) {
            articleIdsByTag = articleTagMapper.selectList(Wrappers.<KbArticleTag>lambdaQuery()
                            .eq(KbArticleTag::getTagId, tagId))
                    .stream().map(KbArticleTag::getArticleId).distinct().toList();
            if (articleIdsByTag.isEmpty()) {
                writeSearchLog(keyword, 0, clientIp);
                return new PageResult<>(0, page, pageSize, Collections.emptyList());
            }
        }

        boolean hasKeyword = StringUtils.hasText(keyword);
        // 分类筛选纳入子分类：把所选分类展开为「自身 + 全部子孙」，用 IN 取代精确等值，
        // 使选根分类也能搜全其下子分类的知识。
        List<Long> categoryIds = categoryId != null ? categoryService.subtreeIds(categoryId) : null;
        var wrapper = Wrappers.<KbArticle>lambdaQuery()
                .eq(KbArticle::getStatus, ONLINE)
                .in(categoryIds != null, KbArticle::getCategoryId, categoryIds)
                .in(articleIdsByTag != null, KbArticle::getId, articleIdsByTag);
        if (hasKeyword) {
            // ngram 全文检索（自带相关度排序），关键词作为参数绑定避免拼接注入。
            wrapper.apply("MATCH(title,summary,content) AGAINST ({0} IN NATURAL LANGUAGE MODE)", keyword.trim());
        } else {
            wrapper.orderByDesc(KbArticle::getViewCount).orderByDesc(KbArticle::getOnlineTime);
        }

        IPage<KbArticle> data = articleMapper.selectPage(new Page<>(page, pageSize), wrapper);
        List<ArticleListItemVO> list = articleService.toListItems(data.getRecords());
        writeSearchLog(keyword, (int) data.getTotal(), clientIp);
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(), list);
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
