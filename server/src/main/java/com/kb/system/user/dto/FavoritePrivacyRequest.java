package com.kb.system.user.dto;

import jakarta.validation.constraints.NotNull;
import lombok.Data;

@Data
public class FavoritePrivacyRequest {

    @NotNull
    private Boolean favoritePrivate;
}
