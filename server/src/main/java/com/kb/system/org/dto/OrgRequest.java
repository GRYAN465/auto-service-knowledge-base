package com.kb.system.org.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class OrgRequest {

    private Long parentId = 0L;

    @NotBlank(message = "机构名称不能为空")
    private String name;

    @NotBlank(message = "机构编码不能为空")
    private String code;

    private String type = "DEPT";
    private Integer sort = 0;
    private String status = "ENABLED";
}
