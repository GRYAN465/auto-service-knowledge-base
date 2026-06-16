package com.kb.system.org.dto;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

@Data
@AllArgsConstructor
public class OrgTreeVO {

    private Long id;
    private Long parentId;
    private String name;
    private String code;
    private String type;
    private Integer sort;
    private String status;
    private List<OrgTreeVO> children;
}
