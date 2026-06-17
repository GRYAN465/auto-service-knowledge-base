package com.kb.openapi.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

@Data
@TableName("sys_api_log")
public class SysApiLog {

    @TableId(type = IdType.AUTO)
    private Long id;
    private Long appId;
    private String appKey;
    private String apiPath;
    private String httpMethod;
    private String requestParam;
    private Integer responseCode;
    private String clientIp;
    private Long costTime;
    private LocalDateTime createTime;
}
