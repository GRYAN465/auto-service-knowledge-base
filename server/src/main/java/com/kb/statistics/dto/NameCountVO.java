package com.kb.statistics.dto;

import lombok.Data;

/**
 * 通用「名称 + 计数」分布项：用于知识状态分布、知识类型分布等 GROUP BY 聚合结果。
 * {@code name} 为分组键（状态码 / 类型码），{@code count} 为该组数量。
 */
@Data
public class NameCountVO {

    private String name;
    private Long count;
}
