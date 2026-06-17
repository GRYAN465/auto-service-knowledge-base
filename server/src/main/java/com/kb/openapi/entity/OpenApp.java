package com.kb.openapi.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

@EqualsAndHashCode(callSuper = true)
@Data
@TableName("open_app")
public class OpenApp extends BaseEntity {

    private String appName;
    private String appKey;
    private String appSecret;
    private String status;
    private Integer rateLimit;
    private String scope;
    private String remark;
}
