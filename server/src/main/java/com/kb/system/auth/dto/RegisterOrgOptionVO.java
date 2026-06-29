package com.kb.system.auth.dto;

import lombok.AllArgsConstructor;
import lombok.Data;

/** 注册页机构下拉选项（树形缩进展示）。 */
@Data
@AllArgsConstructor
public class RegisterOrgOptionVO {

    private Long id;
    private String label;
}
