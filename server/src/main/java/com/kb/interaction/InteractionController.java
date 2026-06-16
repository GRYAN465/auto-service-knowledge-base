package com.kb.interaction;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.interaction.dto.CommentRequest;
import com.kb.interaction.dto.CommentVO;
import com.kb.interaction.dto.FavoriteRequest;
import com.kb.interaction.dto.FavoriteVO;
import com.kb.interaction.dto.InteractionStateVO;
import com.kb.interaction.dto.LikeRequest;
import com.kb.interaction.dto.ShareRequest;
import com.kb.interaction.dto.ShareVO;
import com.kb.interaction.dto.UserOptionVO;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

/**
 * 知识互动（模块 6）：收藏 / 点赞·点踩 / 评论 / 分享 + 站内通知（收到的分享）。
 *
 * <p>权限码：变更类用对应 {@code interaction:*}；门户阅读者可见的读端点
 * （评论列表、互动初态）用 {@code knowledge:search}，与检索/浏览一致。
 */
@Tag(name = "知识互动")
@RestController
@RequestMapping("/interaction")
public class InteractionController {

    private final InteractionService interactionService;

    public InteractionController(InteractionService interactionService) {
        this.interactionService = interactionService;
    }

    // ---------------------------------------------------------------- 收藏

    @Operation(summary = "收藏知识")
    @PostMapping("/favorite")
    @PreAuthorize("hasAuthority('interaction:favorite')")
    public Result<Void> favorite(@RequestBody FavoriteRequest request) {
        interactionService.favorite(request);
        return Result.ok();
    }

    @Operation(summary = "取消收藏")
    @DeleteMapping("/favorite/{articleId}")
    @PreAuthorize("hasAuthority('interaction:favorite')")
    public Result<Void> unfavorite(@PathVariable Long articleId) {
        interactionService.unfavorite(articleId);
        return Result.ok();
    }

    @Operation(summary = "我的收藏")
    @GetMapping("/favorite")
    @PreAuthorize("hasAuthority('interaction:favorite')")
    public Result<PageResult<FavoriteVO>> myFavorites(@RequestParam(defaultValue = "1") long page,
                                                      @RequestParam(defaultValue = "20") long pageSize) {
        return Result.ok(interactionService.myFavorites(page, pageSize));
    }

    // ---------------------------------------------------------------- 点赞/点踩

    @Operation(summary = "点赞/点踩（ARTICLE 或 COMMENT）")
    @PostMapping("/like")
    @PreAuthorize("hasAuthority('interaction:like')")
    public Result<InteractionStateVO> like(@RequestBody LikeRequest request) {
        return Result.ok(interactionService.like(request));
    }

    // ---------------------------------------------------------------- 评论

    @Operation(summary = "发表评论/回复")
    @PostMapping("/comment")
    @PreAuthorize("hasAuthority('interaction:comment')")
    public Result<CommentVO> comment(@RequestBody CommentRequest request) {
        return Result.ok(interactionService.comment(request));
    }

    @Operation(summary = "删除本人评论")
    @DeleteMapping("/comment/{id}")
    @PreAuthorize("hasAuthority('interaction:comment')")
    public Result<Void> deleteComment(@PathVariable Long id) {
        interactionService.deleteComment(id);
        return Result.ok();
    }

    @Operation(summary = "某知识的评论树（门户阅读者可见）")
    @GetMapping("/comment")
    @PreAuthorize("hasAuthority('knowledge:search')")
    public Result<List<CommentVO>> comments(@RequestParam Long articleId) {
        return Result.ok(interactionService.comments(articleId));
    }

    // ---------------------------------------------------------------- 分享 / 站内通知

    @Operation(summary = "分享知识给同事")
    @PostMapping("/share")
    @PreAuthorize("hasAuthority('interaction:share')")
    public Result<Void> share(@RequestBody ShareRequest request) {
        interactionService.share(request);
        return Result.ok();
    }

    @Operation(summary = "收到的分享（站内通知收件箱）")
    @GetMapping("/share/inbox")
    @PreAuthorize("hasAuthority('interaction:share')")
    public Result<PageResult<ShareVO>> shareInbox(@RequestParam(defaultValue = "1") long page,
                                                  @RequestParam(defaultValue = "20") long pageSize) {
        return Result.ok(interactionService.shareInbox(page, pageSize));
    }

    @Operation(summary = "我发出的分享")
    @GetMapping("/share/sent")
    @PreAuthorize("hasAuthority('interaction:share')")
    public Result<PageResult<ShareVO>> shareSent(@RequestParam(defaultValue = "1") long page,
                                                 @RequestParam(defaultValue = "20") long pageSize) {
        return Result.ok(interactionService.shareSent(page, pageSize));
    }

    @Operation(summary = "标记分享为已读")
    @PutMapping("/share/{id}/read")
    @PreAuthorize("hasAuthority('interaction:share')")
    public Result<Void> markShareRead(@PathVariable Long id) {
        interactionService.markShareRead(id);
        return Result.ok();
    }

    @Operation(summary = "未读分享数")
    @GetMapping("/share/unread-count")
    @PreAuthorize("hasAuthority('interaction:share')")
    public Result<Long> unreadShareCount() {
        return Result.ok(interactionService.unreadShareCount());
    }

    @Operation(summary = "可选收件人（启用用户 id+姓名）")
    @GetMapping("/users")
    @PreAuthorize("hasAuthority('interaction:share')")
    public Result<List<UserOptionVO>> shareUsers() {
        return Result.ok(interactionService.shareUsers());
    }

    // ---------------------------------------------------------------- 状态

    @Operation(summary = "某知识对当前用户的互动初态")
    @GetMapping("/state")
    @PreAuthorize("hasAuthority('knowledge:search')")
    public Result<InteractionStateVO> state(@RequestParam Long articleId) {
        return Result.ok(interactionService.state(articleId));
    }
}
