package com.kb.openapi.dto;

import lombok.AllArgsConstructor;
import lombok.Data;

@Data
@AllArgsConstructor
public class ResetSecretResponse {

    private String appSecret;
}
