package com.kb.statistics.dto;

import lombok.Data;

/**
 * 热门搜索词行（{@code GET /statistics/hot-keyword}）：kb_search_log 按 keyword 聚合。
 *
 * <p>{@code zeroCount} 为该词命中 0 结果的检索次数——揭示「用户在搜但库里没有」的内容缺口。
 */
@Data
public class HotKeywordVO {

    private String keyword;
    private Long searchCount;
    private Long zeroCount;
}
