package com.kb.system.user.dto;

import jakarta.validation.constraints.NotBlank;
import lombok.Data;

@Data
public class StatusRequest {

    @NotBlank(message = "状态不能为空")
    private String status;
}
