package com.kb.recommend;

import com.kb.common.Result;
import com.kb.recommend.dto.RecommendHomeVO;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.util.StringUtils;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.Arrays;
import java.util.List;
import java.util.Objects;

/**
 * 首页个性化推荐（向量相似度 + 热度加权）。
 */
@Tag(name = "首页推荐")
@RestController
@RequestMapping("/knowledge/recommend")
public class RecommendController {

    private final RecommendService recommendService;

    public RecommendController(RecommendService recommendService) {
        this.recommendService = recommendService;
    }

    @Operation(summary = "首页推荐知识（行为画像 + 向量匹配 + 热度）")
    @GetMapping("/home")
    @PreAuthorize("hasAuthority('knowledge:search')")
    public Result<RecommendHomeVO> home(@RequestParam(required = false) String pinnedTagIds,
                                        @RequestParam(defaultValue = "5") int limit) {
        return Result.ok(recommendService.homeRecommend(parsePinnedTagIds(pinnedTagIds), limit));
    }

    private static List<Long> parsePinnedTagIds(String pinnedTagIds) {
        if (!StringUtils.hasText(pinnedTagIds)) {
            return List.of();
        }
        return Arrays.stream(pinnedTagIds.split(","))
                .map(String::trim)
                .filter(StringUtils::hasText)
                .map(s -> {
                    try {
                        return Long.parseLong(s);
                    } catch (NumberFormatException e) {
                        return null;
                    }
                })
                .filter(Objects::nonNull)
                .distinct()
                .toList();
    }
}
