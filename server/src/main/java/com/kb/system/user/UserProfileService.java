package com.kb.system.user;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.common.BusinessException;
import com.kb.common.PageResult;
import com.kb.common.ResultCode;
import com.kb.interaction.entity.KbFavorite;
import com.kb.interaction.mapper.KbFavoriteMapper;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.search.SearchService;
import com.kb.knowledge.search.dto.SearchArticleVO;
import com.kb.security.SecurityUtils;
import com.kb.system.user.dto.FavoritePrivacyRequest;
import com.kb.system.user.dto.UserProfileVO;
import com.kb.system.user.entity.SysUser;
import com.kb.system.user.mapper.SysUserMapper;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.stream.Collectors;

/**
 * 用户资料：个人中心 / 他人主页 / 收藏隐私。
 */
@Service
public class UserProfileService {

    private static final String ONLINE = "ONLINE";

    private final SysUserMapper userMapper;
    private final KbArticleMapper articleMapper;
    private final KbFavoriteMapper favoriteMapper;
    private final SearchService searchService;

    public UserProfileService(SysUserMapper userMapper, KbArticleMapper articleMapper,
                              KbFavoriteMapper favoriteMapper, SearchService searchService) {
        this.userMapper = userMapper;
        this.articleMapper = articleMapper;
        this.favoriteMapper = favoriteMapper;
        this.searchService = searchService;
    }

    public UserProfileVO getProfile(Long userId, Long viewerId) {
        SysUser user = userMapper.selectById(userId);
        if (user == null || !"ENABLED".equals(user.getStatus())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户不存在");
        }
        boolean self = Objects.equals(userId, viewerId);
        boolean favoritePrivate = user.getFavoritePrivate() != null && user.getFavoritePrivate() == 1;

        UserProfileVO vo = new UserProfileVO();
        vo.setId(user.getId());
        vo.setUsername(user.getUsername());
        vo.setRealName(user.getRealName());
        vo.setSelf(self);
        vo.setOnlineArticleCount(articleMapper.selectCount(Wrappers.<KbArticle>lambdaQuery()
                .eq(KbArticle::getAuthorId, userId)
                .eq(KbArticle::getStatus, ONLINE)));
        if (self) {
            vo.setFavoritePrivate(favoritePrivate);
        }
        vo.setFavoritesVisible(self || !favoritePrivate);
        return vo;
    }

    public PageResult<SearchArticleVO> userFavorites(Long userId, long page, long pageSize, Long viewerId) {
        UserProfileVO profile = getProfile(userId, viewerId);
        if (!Boolean.TRUE.equals(profile.getFavoritesVisible())) {
            throw new BusinessException(ResultCode.FORBIDDEN, "对方已将收藏设为隐私");
        }
        var data = favoriteMapper.selectPage(new com.baomidou.mybatisplus.extension.plugins.pagination.Page<>(page, pageSize),
                Wrappers.<KbFavorite>lambdaQuery()
                        .eq(KbFavorite::getUserId, userId)
                        .orderByDesc(KbFavorite::getCreateTime)
                        .orderByDesc(KbFavorite::getId));
        List<KbFavorite> rows = data.getRecords();
        if (rows.isEmpty()) {
            return new PageResult<>(0, page, pageSize, Collections.emptyList());
        }
        Map<Long, KbArticle> articles = articleMapper.selectBatchIds(rows.stream()
                        .map(KbFavorite::getArticleId).collect(Collectors.toSet())).stream()
                .filter(a -> ONLINE.equals(a.getStatus()))
                .collect(Collectors.toMap(KbArticle::getId, a -> a, (a, b) -> a));
        List<KbArticle> ordered = rows.stream()
                .map(f -> articles.get(f.getArticleId()))
                .filter(Objects::nonNull)
                .toList();
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(),
                searchService.toSearchItems(ordered));
    }

    @Transactional
    public void updateFavoritePrivacy(FavoritePrivacyRequest request) {
        Long userId = SecurityUtils.getUserIdOrNull();
        if (userId == null) {
            throw new BusinessException(ResultCode.UNAUTHORIZED, "未登录");
        }
        SysUser user = userMapper.selectById(userId);
        if (user == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户不存在");
        }
        user.setFavoritePrivate(Boolean.TRUE.equals(request.getFavoritePrivate()) ? 1 : 0);
        userMapper.updateById(user);
    }
}
