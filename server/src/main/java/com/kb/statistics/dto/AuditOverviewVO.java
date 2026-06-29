package com.kb.statistics.dto;

import lombok.Data;

import java.util.List;

/** 审核运营概览。 */
@Data
public class AuditOverviewVO {

    private Long pendingAudit;
    private Long passCount;
    private Long rejectCount;
    /** 通过率 0~1，无审核记录时为 null。 */
    private Double passRate;
    private List<NameCountVO> resultDist;
}
