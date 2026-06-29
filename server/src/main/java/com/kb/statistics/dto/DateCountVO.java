package com.kb.statistics.dto;

import lombok.Data;

/** 按日计数（yyyy-MM-dd → count），用于趋势折线图。 */
@Data
public class DateCountVO {

    private String date;
    private Long count;
}
