package com.kb.system.auth;

import com.kb.common.BusinessException;
import com.kb.common.Result;
import com.kb.common.ResultCode;
import com.kb.security.LoginUser;
import com.kb.security.SecurityUtils;
import com.kb.system.auth.dto.LoginRequest;
import com.kb.system.auth.dto.MeResponse;
import com.kb.system.auth.dto.TokenResponse;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.Valid;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

/**
 * 鉴权接口（见《API契约.md》§2）。登录与当前用户均落到 {@link AuthService}（DB + BCrypt + RBAC）。
 */
@Tag(name = "鉴权", description = "登录 / 注销 / 当前用户")
@RestController
@RequestMapping("/auth")
public class AuthController {

    private final AuthService authService;

    public AuthController(AuthService authService) {
        this.authService = authService;
    }

    @Operation(summary = "登录，返回 JWT")
    @PostMapping("/login")
    public Result<TokenResponse> login(@RequestBody @Valid LoginRequest request, HttpServletRequest http) {
        return Result.ok(authService.login(request.getUsername(), request.getPassword(), clientIp(http)));
    }

    @Operation(summary = "注销")
    @PostMapping("/logout")
    public Result<Void> logout() {
        // 无状态 JWT：客户端丢弃 token 即可；如需服务端失效可后续加黑名单。
        return Result.ok();
    }

    @Operation(summary = "当前用户 + 菜单树 + 权限码")
    @GetMapping("/me")
    public Result<MeResponse> me() {
        LoginUser loginUser = SecurityUtils.getLoginUserOrNull();
        if (loginUser == null) {
            throw new BusinessException(ResultCode.UNAUTHORIZED, "未登录");
        }
        return Result.ok(authService.me(loginUser.getUserId()));
    }

    /** 取客户端 IP（优先 X-Forwarded-For 首段，便于经反向代理时记录真实 IP）。 */
    private static String clientIp(HttpServletRequest request) {
        String xff = request.getHeader("X-Forwarded-For");
        if (xff != null && !xff.isBlank()) {
            int comma = xff.indexOf(',');
            return comma > 0 ? xff.substring(0, comma).trim() : xff.trim();
        }
        return request.getRemoteAddr();
    }
}
