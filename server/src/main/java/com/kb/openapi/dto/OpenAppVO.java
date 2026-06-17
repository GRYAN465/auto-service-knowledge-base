package com.kb.openapi.dto;

import com.fasterxml.jackson.annotation.JsonInclude;
import com.kb.openapi.entity.OpenApp;
import lombok.Data;

import java.time.LocalDateTime;

@Data
@JsonInclude(JsonInclude.Include.NON_NULL)
public class OpenAppVO {

    private Long id;
    private String appName;
    private String appKey;
    private String appSecret;
    private String appSecretMasked;
    private String status;
    private Integer rateLimit;
    private String scope;
    private String remark;
    private LocalDateTime createTime;
    private LocalDateTime updateTime;

    public static OpenAppVO from(OpenApp app) {
        OpenAppVO vo = new OpenAppVO();
        vo.setId(app.getId());
        vo.setAppName(app.getAppName());
        vo.setAppKey(app.getAppKey());
        vo.setAppSecretMasked(mask(app.getAppSecret()));
        vo.setStatus(app.getStatus());
        vo.setRateLimit(app.getRateLimit());
        vo.setScope(app.getScope());
        vo.setRemark(app.getRemark());
        vo.setCreateTime(app.getCreateTime());
        vo.setUpdateTime(app.getUpdateTime());
        return vo;
    }

    private static String mask(String secret) {
        if (secret == null || secret.length() <= 8) {
            return "********";
        }
        return secret.substring(0, 4) + "****" + secret.substring(secret.length() - 4);
    }
}
