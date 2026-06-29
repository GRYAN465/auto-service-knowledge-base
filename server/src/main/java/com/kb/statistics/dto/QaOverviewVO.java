package com.kb.statistics.dto;

import lombok.Data;

import java.util.List;

/** 智能问答运营概览（二期）。 */
@Data
public class QaOverviewVO {

    private Integer days;
    private Long sessionTotal;
    private Long messageTotal;
    private Long helpfulCount;
    private Long unhelpfulCount;
    /** 有用反馈占比 0~1，无反馈时为 null。 */
    private Double helpfulRate;
    private List<DateCountVO> sessionTrend;
}
