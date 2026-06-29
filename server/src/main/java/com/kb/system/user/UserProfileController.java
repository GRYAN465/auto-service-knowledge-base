package com.kb.system.user;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.knowledge.search.dto.SearchArticleVO;
import com.kb.security.SecurityUtils;
import com.kb.system.user.dto.FavoritePrivacyRequest;
import com.kb.system.user.dto.UserProfileVO;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

/**
 * 用户资料接口：个人中心 / 他人主页 / 收藏隐私。
 */
@Tag(name = "用户资料")
@RestController
@RequestMapping("/user/profile")
public class UserProfileController {

    private final UserProfileService profileService;

    public UserProfileController(UserProfileService profileService) {
        this.profileService = profileService;
    }

    @Operation(summary = "用户资料")
    @GetMapping("/{id}")
    @PreAuthorize("isAuthenticated()")
    public Result<UserProfileVO> profile(@PathVariable Long id) {
        Long viewerId = SecurityUtils.getUserIdOrNull();
        return Result.ok(profileService.getProfile(id, viewerId));
    }

    @Operation(summary = "用户公开收藏（受隐私控制）")
    @GetMapping("/{id}/favorites")
    @PreAuthorize("hasAuthority('knowledge:search')")
    public Result<PageResult<SearchArticleVO>> favorites(@PathVariable Long id,
                                                           @RequestParam(defaultValue = "1") long page,
                                                           @RequestParam(defaultValue = "20") long pageSize) {
        Long viewerId = SecurityUtils.getUserIdOrNull();
        return Result.ok(profileService.userFavorites(id, page, pageSize, viewerId));
    }

    @Operation(summary = "设置收藏隐私（仅本人）")
    @PutMapping("/favorite-privacy")
    @PreAuthorize("isAuthenticated()")
    public Result<Void> favoritePrivacy(@RequestBody @Valid FavoritePrivacyRequest request) {
        profileService.updateFavoritePrivacy(request);
        return Result.ok();
    }
}
