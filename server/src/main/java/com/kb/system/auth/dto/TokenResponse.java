package com.kb.system.auth.dto;

import lombok.AllArgsConstructor;
import lombok.Data;

@Data
@AllArgsConstructor
public class TokenResponse {

    private String token;

    /** token 有效期（秒）。 */
    private long expiresIn;
}
