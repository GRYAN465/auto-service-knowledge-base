package com.kb.statistics.dto;

import lombok.Data;

import java.util.List;

/** 平台活跃与内容生产日趋势。 */
@Data
public class TrendVO {

    private Integer days;
    /** day / month */
    private String granularity;
    private Integer year;
    private List<DateCountVO> views;
    private List<DateCountVO> searches;
    private List<DateCountVO> newArticles;
    private List<DateCountVO> onlineArticles;
}
