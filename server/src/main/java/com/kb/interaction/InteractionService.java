package com.kb.interaction;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.BusinessException;
import com.kb.common.PageResult;
import com.kb.common.ResultCode;
import com.kb.interaction.dto.CommentRequest;
import com.kb.interaction.dto.CommentVO;
import com.kb.interaction.dto.FavoriteRequest;
import com.kb.interaction.dto.FavoriteVO;
import com.kb.interaction.dto.InteractionStateVO;
import com.kb.interaction.dto.LikeRequest;
import com.kb.interaction.dto.ShareRequest;
import com.kb.interaction.dto.ShareVO;
import com.kb.interaction.dto.UserOptionVO;
import com.kb.interaction.entity.KbComment;
import com.kb.interaction.entity.KbFavorite;
import com.kb.interaction.entity.KbLike;
import com.kb.interaction.entity.KbShare;
import com.kb.interaction.mapper.KbCommentMapper;
import com.kb.interaction.mapper.KbFavoriteMapper;
import com.kb.interaction.mapper.KbLikeMapper;
import com.kb.interaction.mapper.KbShareMapper;
import com.kb.knowledge.article.entity.KbArticle;
import com.kb.knowledge.article.mapper.KbArticleMapper;
import com.kb.knowledge.category.entity.KbCategory;
import com.kb.knowledge.category.mapper.KbCategoryMapper;
import com.kb.security.SecurityUtils;
import com.kb.system.user.entity.SysUser;
import com.kb.system.user.mapper.SysUserMapper;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.util.StringUtils;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * 知识互动业务（模块 6）：收藏 / 点赞·点踩 / 评论 / 分享 + 站内通知（收到的分享）。
 *
 * <p>计数自增镜像 {@code ArticleService.viewDetail}：用 {@code setSql} 空实体更新，
 * 不触发审计字段自动填充、不动 {@code update_time}，避免互动把知识顶到管理列表最前；
 * 自减一律 {@code GREATEST(x-1,0)} 防负。当前用户走 {@link SecurityUtils#getUserIdOrNull()}。
 */
@Service
public class InteractionService {

    private static final String TARGET_ARTICLE = "ARTICLE";
    private static final String TARGET_COMMENT = "COMMENT";
    private static final String COMMENT_NORMAL = "NORMAL";
    private static final String SHARE_TYPE_USER = "USER";

    private final KbFavoriteMapper favoriteMapper;
    private final KbShareMapper shareMapper;
    private final KbLikeMapper likeMapper;
    private final KbCommentMapper commentMapper;
    private final KbArticleMapper articleMapper;
    private final SysUserMapper userMapper;
    private final KbCategoryMapper categoryMapper;

    public InteractionService(KbFavoriteMapper favoriteMapper, KbShareMapper shareMapper,
                              KbLikeMapper likeMapper, KbCommentMapper commentMapper,
                              KbArticleMapper articleMapper, SysUserMapper userMapper,
                              KbCategoryMapper categoryMapper) {
        this.favoriteMapper = favoriteMapper;
        this.shareMapper = shareMapper;
        this.likeMapper = likeMapper;
        this.commentMapper = commentMapper;
        this.articleMapper = articleMapper;
        this.userMapper = userMapper;
        this.categoryMapper = categoryMapper;
    }

    // ---------------------------------------------------------------- 收藏

    @Transactional
    public void favorite(FavoriteRequest request) {
        if (request == null || request.getArticleId() == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "缺少 articleId");
        }
        Long articleId = request.getArticleId();
        getArticle(articleId);
        Long userId = currentUserId();

        KbFavorite existed = favoriteMapper.selectOne(Wrappers.<KbFavorite>lambdaQuery()
                .eq(KbFavorite::getArticleId, articleId)
                .eq(KbFavorite::getUserId, userId)
                .last("limit 1"));
        if (existed != null) {
            // 幂等：已收藏则仅更新收藏夹（如有变化），不重复计数。
            if (StringUtils.hasText(request.getFolder()) && !request.getFolder().equals(existed.getFolder())) {
                existed.setFolder(request.getFolder());
                favoriteMapper.updateById(existed);
            }
            return;
        }
        KbFavorite favorite = new KbFavorite();
        favorite.setArticleId(articleId);
        favorite.setUserId(userId);
        favorite.setFolder(request.getFolder());
        favorite.setCreateTime(LocalDateTime.now());
        favoriteMapper.insert(favorite);
        incrArticle(articleId, "favorite_count = favorite_count + 1");
    }

    @Transactional
    public void unfavorite(Long articleId) {
        if (articleId == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "缺少 articleId");
        }
        Long userId = currentUserId();
        int removed = favoriteMapper.delete(Wrappers.<KbFavorite>lambdaQuery()
                .eq(KbFavorite::getArticleId, articleId)
                .eq(KbFavorite::getUserId, userId));
        if (removed > 0) {
            incrArticle(articleId, "favorite_count = GREATEST(favorite_count - 1, 0)");
        }
    }

    public PageResult<FavoriteVO> myFavorites(long page, long pageSize) {
        Long userId = currentUserId();
        IPage<KbFavorite> data = favoriteMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<KbFavorite>lambdaQuery()
                        .eq(KbFavorite::getUserId, userId)
                        .orderByDesc(KbFavorite::getCreateTime)
                        .orderByDesc(KbFavorite::getId));
        List<KbFavorite> rows = data.getRecords();
        Map<Long, KbArticle> articles = articleMap(rows.stream()
                .map(KbFavorite::getArticleId).collect(Collectors.toSet()));
        Map<Long, String> categoryNames = categoryNameMap(articles.values().stream()
                .map(KbArticle::getCategoryId).filter(Objects::nonNull).collect(Collectors.toSet()));

        List<FavoriteVO> list = rows.stream().map(f -> {
            FavoriteVO vo = new FavoriteVO();
            vo.setId(f.getId());
            vo.setArticleId(f.getArticleId());
            vo.setFolder(f.getFolder());
            vo.setCreateTime(f.getCreateTime());
            KbArticle a = articles.get(f.getArticleId());
            if (a != null) {
                vo.setTitle(a.getTitle());
                vo.setKnowledgeType(a.getKnowledgeType());
                vo.setCategoryName(categoryNames.get(a.getCategoryId()));
            }
            return vo;
        }).toList();
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(), list);
    }

    // ---------------------------------------------------------------- 点赞/点踩

    @Transactional
    public InteractionStateVO like(LikeRequest request) {
        if (request == null || request.getTargetType() == null || request.getTargetId() == null
                || request.getType() == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "缺少 targetType/targetId/type");
        }
        String targetType = request.getTargetType().toUpperCase();
        if (!TARGET_ARTICLE.equals(targetType) && !TARGET_COMMENT.equals(targetType)) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "targetType 须为 ARTICLE 或 COMMENT");
        }
        int type = request.getType();
        if (type != 1 && type != -1) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "type 须为 1（赞）或 -1（踩）");
        }
        Long targetId = request.getTargetId();
        // 校验目标存在
        if (TARGET_ARTICLE.equals(targetType)) {
            getArticle(targetId);
        } else {
            getComment(targetId);
        }
        Long userId = currentUserId();

        KbLike existed = likeMapper.selectOne(Wrappers.<KbLike>lambdaQuery()
                .eq(KbLike::getTargetType, targetType)
                .eq(KbLike::getTargetId, targetId)
                .eq(KbLike::getUserId, userId)
                .last("limit 1"));

        if (existed == null) {
            // 新表态：插入 + 对应计数 +1
            KbLike like = new KbLike();
            like.setTargetType(targetType);
            like.setTargetId(targetId);
            like.setUserId(userId);
            like.setType(type);
            like.setCreateTime(LocalDateTime.now());
            likeMapper.insert(like);
            applyLikeCount(targetType, targetId, type, +1);
        } else if (existed.getType() == type) {
            // 再次点击同一表态 → 取消：删除 + 对应计数 -1
            likeMapper.deleteById(existed.getId());
            applyLikeCount(targetType, targetId, type, -1);
        } else {
            // 改表态（赞↔踩）：旧表态计数 -1，新表态计数 +1
            int oldType = existed.getType();
            existed.setType(type);
            likeMapper.updateById(existed);
            applyLikeCount(targetType, targetId, oldType, -1);
            applyLikeCount(targetType, targetId, type, +1);
        }

        return targetState(targetType, targetId, userId);
    }

    /** 调整目标的赞/踩计数：article 同时维护 like_count/dislike_count；comment 仅 like_count 跟随 type=1。 */
    private void applyLikeCount(String targetType, Long targetId, int type, int delta) {
        if (TARGET_ARTICLE.equals(targetType)) {
            String column = type == 1 ? "like_count" : "dislike_count";
            String sql = delta > 0
                    ? column + " = " + column + " + 1"
                    : column + " = GREATEST(" + column + " - 1, 0)";
            incrArticle(targetId, sql);
        } else {
            // 评论只统计赞
            if (type == 1) {
                String sql = delta > 0
                        ? "like_count = like_count + 1"
                        : "like_count = GREATEST(like_count - 1, 0)";
                incrComment(targetId, sql);
            }
        }
    }

    // ---------------------------------------------------------------- 评论

    @Transactional
    public CommentVO comment(CommentRequest request) {
        if (request == null || request.getArticleId() == null || !StringUtils.hasText(request.getContent())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "缺少 articleId 或评论内容");
        }
        Long articleId = request.getArticleId();
        getArticle(articleId);
        long parentId = request.getParentId() == null ? 0L : request.getParentId();
        if (parentId != 0L) {
            KbComment parent = getComment(parentId);
            if (!Objects.equals(parent.getArticleId(), articleId)) {
                throw new BusinessException(ResultCode.PARAM_ERROR, "父评论不属于该知识");
            }
        }
        Long userId = currentUserId();

        KbComment comment = new KbComment();
        comment.setArticleId(articleId);
        comment.setUserId(userId);
        comment.setParentId(parentId);
        comment.setContent(request.getContent());
        comment.setStatus(COMMENT_NORMAL);
        comment.setLikeCount(0L);
        comment.setCreateTime(LocalDateTime.now());
        commentMapper.insert(comment);
        incrArticle(articleId, "comment_count = comment_count + 1");

        CommentVO vo = toCommentVO(comment, userNameMap(Set.of(userId)));
        return vo;
    }

    @Transactional
    public void deleteComment(Long id) {
        KbComment comment = getComment(id);
        Long userId = currentUserId();
        if (!Objects.equals(comment.getUserId(), userId)) {
            throw new BusinessException(ResultCode.FORBIDDEN, "只能删除本人的评论");
        }
        commentMapper.deleteById(id); // @TableLogic 逻辑删
        incrArticle(comment.getArticleId(), "comment_count = GREATEST(comment_count - 1, 0)");
    }

    public List<CommentVO> comments(Long articleId) {
        if (articleId == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "缺少 articleId");
        }
        List<KbComment> rows = commentMapper.selectList(Wrappers.<KbComment>lambdaQuery()
                .eq(KbComment::getArticleId, articleId)
                .eq(KbComment::getStatus, COMMENT_NORMAL)
                .orderByAsc(KbComment::getCreateTime)
                .orderByAsc(KbComment::getId));
        if (rows.isEmpty()) {
            return Collections.emptyList();
        }
        Map<Long, String> userNames = userNameMap(rows.stream()
                .map(KbComment::getUserId).filter(Objects::nonNull).collect(Collectors.toSet()));

        // 内存组装树：先建 id→VO，再按 parentId 挂载。
        Map<Long, CommentVO> byId = new LinkedHashMap<>();
        for (KbComment c : rows) {
            byId.put(c.getId(), toCommentVO(c, userNames));
        }
        List<CommentVO> roots = new ArrayList<>();
        for (KbComment c : rows) {
            CommentVO vo = byId.get(c.getId());
            long parentId = c.getParentId() == null ? 0L : c.getParentId();
            CommentVO parent = parentId == 0L ? null : byId.get(parentId);
            if (parent != null) {
                parent.getChildren().add(vo);
            } else {
                roots.add(vo);
            }
        }
        return roots;
    }

    // ---------------------------------------------------------------- 分享 / 站内通知

    @Transactional
    public void share(ShareRequest request) {
        if (request == null || request.getArticleId() == null || request.getToUserId() == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "缺少 articleId 或 toUserId");
        }
        getArticle(request.getArticleId());
        SysUser toUser = userMapper.selectById(request.getToUserId());
        if (toUser == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "收件人不存在");
        }
        if (!"ENABLED".equals(toUser.getStatus())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "收件人已被停用");
        }
        KbShare share = new KbShare();
        share.setArticleId(request.getArticleId());
        share.setFromUserId(currentUserId());
        share.setToUserId(request.getToUserId());
        share.setShareType(SHARE_TYPE_USER);
        share.setMessage(request.getMessage());
        share.setReadStatus(0);
        share.setCreateTime(LocalDateTime.now());
        shareMapper.insert(share);
    }

    public PageResult<ShareVO> shareInbox(long page, long pageSize) {
        Long userId = currentUserId();
        IPage<KbShare> data = shareMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<KbShare>lambdaQuery()
                        .eq(KbShare::getToUserId, userId)
                        .orderByDesc(KbShare::getCreateTime)
                        .orderByDesc(KbShare::getId));
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(), toShareVOs(data.getRecords()));
    }

    public PageResult<ShareVO> shareSent(long page, long pageSize) {
        Long userId = currentUserId();
        IPage<KbShare> data = shareMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<KbShare>lambdaQuery()
                        .eq(KbShare::getFromUserId, userId)
                        .orderByDesc(KbShare::getCreateTime)
                        .orderByDesc(KbShare::getId));
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(), toShareVOs(data.getRecords()));
    }

    @Transactional
    public void markShareRead(Long id) {
        Long userId = currentUserId();
        shareMapper.update(null, Wrappers.<KbShare>lambdaUpdate()
                .set(KbShare::getReadStatus, 1)
                .eq(KbShare::getId, id)
                .eq(KbShare::getToUserId, userId));
    }

    public long unreadShareCount() {
        Long userId = currentUserId();
        return shareMapper.selectCount(Wrappers.<KbShare>lambdaQuery()
                .eq(KbShare::getToUserId, userId)
                .eq(KbShare::getReadStatus, 0));
    }

    public List<UserOptionVO> shareUsers() {
        Long userId = currentUserId();
        return userMapper.selectList(Wrappers.<SysUser>lambdaQuery()
                        .eq(SysUser::getStatus, "ENABLED")
                        .ne(userId != null, SysUser::getId, userId)
                        .orderByAsc(SysUser::getId))
                .stream()
                .map(u -> new UserOptionVO(u.getId(),
                        StringUtils.hasText(u.getRealName()) ? u.getRealName() : u.getUsername()))
                .toList();
    }

    // ---------------------------------------------------------------- 状态

    public InteractionStateVO state(Long articleId) {
        if (articleId == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "缺少 articleId");
        }
        getArticle(articleId);
        return targetState(TARGET_ARTICLE, articleId, currentUserId());
    }

    // ---------------------------------------------------------------- 私有辅助

    private InteractionStateVO targetState(String targetType, Long targetId, Long userId) {
        InteractionStateVO vo = new InteractionStateVO();
        KbLike myLike = likeMapper.selectOne(Wrappers.<KbLike>lambdaQuery()
                .eq(KbLike::getTargetType, targetType)
                .eq(KbLike::getTargetId, targetId)
                .eq(KbLike::getUserId, userId)
                .last("limit 1"));
        vo.setMyLikeType(myLike == null ? 0 : myLike.getType());

        if (TARGET_ARTICLE.equals(targetType)) {
            KbArticle a = articleMapper.selectById(targetId);
            boolean favorited = favoriteMapper.selectCount(Wrappers.<KbFavorite>lambdaQuery()
                    .eq(KbFavorite::getArticleId, targetId)
                    .eq(KbFavorite::getUserId, userId)) > 0;
            vo.setFavorited(favorited);
            if (a != null) {
                vo.setLikeCount(a.getLikeCount());
                vo.setDislikeCount(a.getDislikeCount());
                vo.setFavoriteCount(a.getFavoriteCount());
                vo.setCommentCount(a.getCommentCount());
            }
        } else {
            KbComment c = commentMapper.selectById(targetId);
            if (c != null) {
                vo.setLikeCount(c.getLikeCount());
            }
        }
        return vo;
    }

    private List<ShareVO> toShareVOs(List<KbShare> rows) {
        if (rows == null || rows.isEmpty()) {
            return Collections.emptyList();
        }
        Map<Long, KbArticle> articles = articleMap(rows.stream()
                .map(KbShare::getArticleId).collect(Collectors.toSet()));
        Set<Long> userIds = new java.util.HashSet<>();
        rows.forEach(s -> {
            if (s.getFromUserId() != null) {
                userIds.add(s.getFromUserId());
            }
            if (s.getToUserId() != null) {
                userIds.add(s.getToUserId());
            }
        });
        Map<Long, String> userNames = userNameMap(userIds);

        return rows.stream().map(s -> {
            ShareVO vo = new ShareVO();
            vo.setId(s.getId());
            vo.setArticleId(s.getArticleId());
            KbArticle a = articles.get(s.getArticleId());
            vo.setArticleTitle(a == null ? null : a.getTitle());
            vo.setFromUserId(s.getFromUserId());
            vo.setFromUserName(userNames.get(s.getFromUserId()));
            vo.setToUserId(s.getToUserId());
            vo.setToUserName(userNames.get(s.getToUserId()));
            vo.setMessage(s.getMessage());
            vo.setReadStatus(s.getReadStatus());
            vo.setCreateTime(s.getCreateTime());
            return vo;
        }).toList();
    }

    private CommentVO toCommentVO(KbComment c, Map<Long, String> userNames) {
        CommentVO vo = new CommentVO();
        vo.setId(c.getId());
        vo.setArticleId(c.getArticleId());
        vo.setUserId(c.getUserId());
        vo.setUserName(userNames.get(c.getUserId()));
        vo.setParentId(c.getParentId());
        vo.setContent(c.getContent());
        vo.setLikeCount(c.getLikeCount());
        vo.setCreateTime(c.getCreateTime());
        return vo;
    }

    private KbArticle getArticle(Long id) {
        KbArticle a = articleMapper.selectById(id);
        if (a == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "知识不存在");
        }
        return a;
    }

    private KbComment getComment(Long id) {
        KbComment c = commentMapper.selectById(id);
        if (c == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "评论不存在");
        }
        return c;
    }

    private void incrArticle(Long articleId, String setSql) {
        articleMapper.update(null, Wrappers.<KbArticle>lambdaUpdate()
                .setSql(setSql)
                .eq(KbArticle::getId, articleId));
    }

    private void incrComment(Long commentId, String setSql) {
        commentMapper.update(null, Wrappers.<KbComment>lambdaUpdate()
                .setSql(setSql)
                .eq(KbComment::getId, commentId));
    }

    private Long currentUserId() {
        Long userId = SecurityUtils.getUserIdOrNull();
        if (userId == null) {
            throw new BusinessException(ResultCode.UNAUTHORIZED, "未登录");
        }
        return userId;
    }

    private Map<Long, KbArticle> articleMap(Set<Long> ids) {
        if (ids == null || ids.isEmpty()) {
            return Collections.emptyMap();
        }
        return articleMapper.selectBatchIds(ids).stream()
                .collect(Collectors.toMap(KbArticle::getId, a -> a, (a, b) -> a));
    }

    private Map<Long, String> categoryNameMap(Set<Long> ids) {
        if (ids == null || ids.isEmpty()) {
            return Collections.emptyMap();
        }
        return categoryMapper.selectBatchIds(ids).stream()
                .collect(Collectors.toMap(KbCategory::getId, KbCategory::getName, (a, b) -> a));
    }

    private Map<Long, String> userNameMap(Set<Long> ids) {
        if (ids == null || ids.isEmpty()) {
            return Collections.emptyMap();
        }
        return userMapper.selectBatchIds(ids).stream().collect(Collectors.toMap(SysUser::getId,
                u -> StringUtils.hasText(u.getRealName()) ? u.getRealName() : u.getUsername(), (a, b) -> a));
    }
}
