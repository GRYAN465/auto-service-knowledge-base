package com.kb.openapi;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.openapi.entity.OpenApp;
import com.kb.openapi.entity.SysApiLog;
import com.kb.openapi.mapper.OpenAppMapper;
import com.kb.openapi.mapper.SysApiLogMapper;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HexFormat;
import java.util.Locale;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;

@Service
public class OpenApiAuthService {

    private static final long MAX_CLOCK_SKEW_SECONDS = 300;
    private static final DateTimeFormatter MINUTE = DateTimeFormatter.ofPattern("yyyyMMddHHmm");

    private final OpenAppMapper appMapper;
    private final SysApiLogMapper apiLogMapper;
    private final ObjectMapper objectMapper;
    private final Map<String, AtomicInteger> minuteCounters = new ConcurrentHashMap<>();
    private final Map<String, Long> nonceExpirations = new ConcurrentHashMap<>();

    public OpenApiAuthService(OpenAppMapper appMapper, SysApiLogMapper apiLogMapper, ObjectMapper objectMapper) {
        this.appMapper = appMapper;
        this.apiLogMapper = apiLogMapper;
        this.objectMapper = objectMapper;
    }

    public OpenApp authenticate(HttpServletRequest request, String scope, String bodyText) {
        String appKey = header(request, "X-App-Key");
        String timestamp = header(request, "X-Timestamp");
        String nonce = header(request, "X-Nonce");
        String signature = header(request, "X-Signature");
        if (!StringUtils.hasText(appKey) || !StringUtils.hasText(timestamp)
                || !StringUtils.hasText(nonce) || !StringUtils.hasText(signature)) {
            throw new BusinessException(ResultCode.UNAUTHORIZED, "缺少开放 API 鉴权头");
        }
        OpenApp app = appMapper.selectOne(Wrappers.<OpenApp>lambdaQuery().eq(OpenApp::getAppKey, appKey));
        if (app == null || !"ENABLED".equals(app.getStatus())) {
            throw new BusinessException(ResultCode.FORBIDDEN, "开放应用不可用");
        }
        assertScope(app, scope);
        assertTimestamp(timestamp);
        String expected = sign(app.getAppSecret(), canonical(request, bodyText, timestamp, nonce));
        if (!MessageDigest.isEqual(expected.getBytes(StandardCharsets.UTF_8),
                signature.getBytes(StandardCharsets.UTF_8))) {
            throw new BusinessException(ResultCode.FORBIDDEN, "签名校验失败");
        }
        assertNonce(app, nonce);
        assertRateLimit(app);
        return app;
    }

    public void record(OpenApp app, String appKey, HttpServletRequest request, Object requestPayload,
                       int responseCode, long costTime) {
        SysApiLog log = new SysApiLog();
        log.setAppId(app == null ? null : app.getId());
        log.setAppKey(StringUtils.hasText(appKey) ? appKey : (app == null ? null : app.getAppKey()));
        log.setApiPath(request.getRequestURI());
        log.setHttpMethod(request.getMethod());
        log.setRequestParam(toText(requestPayload, request.getQueryString()));
        log.setResponseCode(responseCode);
        log.setClientIp(clientIp(request));
        log.setCostTime(costTime);
        log.setCreateTime(LocalDateTime.now());
        apiLogMapper.insert(log);
    }

    public String clientIp(HttpServletRequest request) {
        String xff = request.getHeader("X-Forwarded-For");
        if (xff != null && !xff.isBlank()) {
            int comma = xff.indexOf(',');
            return comma > 0 ? xff.substring(0, comma).trim() : xff.trim();
        }
        return request.getRemoteAddr();
    }

    private void assertScope(OpenApp app, String scope) {
        boolean allowed = Arrays.stream((app.getScope() == null ? "" : app.getScope()).split(","))
                .map(String::trim)
                .anyMatch(s -> s.equalsIgnoreCase(scope));
        if (!allowed) {
            throw new BusinessException(ResultCode.FORBIDDEN, "应用无该接口权限");
        }
    }

    private static void assertTimestamp(String timestamp) {
        long ts;
        try {
            ts = Long.parseLong(timestamp);
        } catch (NumberFormatException e) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "时间戳格式错误");
        }
        long now = System.currentTimeMillis() / 1000;
        if (Math.abs(now - ts) > MAX_CLOCK_SKEW_SECONDS) {
            throw new BusinessException(ResultCode.FORBIDDEN, "请求已过期");
        }
    }

    private void assertRateLimit(OpenApp app) {
        int limit = app.getRateLimit() == null ? 60 : Math.max(1, app.getRateLimit());
        String key = app.getId() + ":" + LocalDateTime.now().format(MINUTE);
        int current = minuteCounters.computeIfAbsent(key, k -> new AtomicInteger()).incrementAndGet();
        if (current > limit) {
            throw new BusinessException(ResultCode.FORBIDDEN, "调用频率超过限制");
        }
        if (minuteCounters.size() > 10000) {
            String prefix = LocalDateTime.now().format(MINUTE);
            minuteCounters.keySet().removeIf(k -> !k.endsWith(prefix));
        }
    }

    private void assertNonce(OpenApp app, String nonce) {
        long now = System.currentTimeMillis() / 1000;
        long expiresAt = now + MAX_CLOCK_SKEW_SECONDS;
        String key = app.getId() + ":" + nonce;
        Long previous = nonceExpirations.putIfAbsent(key, expiresAt);
        if (previous != null) {
            if (previous >= now) {
                throw new BusinessException(ResultCode.FORBIDDEN, "重复请求");
            }
            nonceExpirations.replace(key, previous, expiresAt);
        }
        if (nonceExpirations.size() > 10000) {
            nonceExpirations.entrySet().removeIf(e -> e.getValue() < now);
        }
    }

    private static String canonical(HttpServletRequest request, String bodyText, String timestamp, String nonce) {
        return request.getMethod().toUpperCase(Locale.ROOT) + "\n"
                + canonicalPath(request) + "\n"
                + canonicalQuery(request) + "\n"
                + sha256(bodyText == null ? "" : bodyText) + "\n"
                + timestamp + "\n"
                + nonce;
    }

    private static String canonicalPath(HttpServletRequest request) {
        String path = request.getServletPath();
        if (StringUtils.hasText(path)) {
            return path;
        }
        String uri = request.getRequestURI();
        String contextPath = request.getContextPath();
        return StringUtils.hasText(contextPath) && uri.startsWith(contextPath)
                ? uri.substring(contextPath.length())
                : uri;
    }

    private static String canonicalQuery(HttpServletRequest request) {
        return request.getParameterMap().entrySet().stream()
                .sorted(Map.Entry.comparingByKey())
                .flatMap(e -> Arrays.stream(e.getValue())
                        .sorted(Comparator.naturalOrder())
                        .map(v -> e.getKey() + "=" + v))
                .collect(Collectors.joining("&"));
    }

    private static String sign(String secret, String content) {
        try {
            Mac mac = Mac.getInstance("HmacSHA256");
            mac.init(new SecretKeySpec(secret.getBytes(StandardCharsets.UTF_8), "HmacSHA256"));
            return HexFormat.of().formatHex(mac.doFinal(content.getBytes(StandardCharsets.UTF_8)));
        } catch (Exception e) {
            throw new IllegalStateException("签名失败", e);
        }
    }

    private static String sha256(String text) {
        try {
            return HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256")
                    .digest(text.getBytes(StandardCharsets.UTF_8)));
        } catch (Exception e) {
            throw new IllegalStateException("摘要失败", e);
        }
    }

    private static String header(HttpServletRequest request, String name) {
        return request.getHeader(name);
    }

    private String toText(Object payload, String fallback) {
        if (payload == null) {
            return fallback;
        }
        if (payload instanceof String str) {
            return str;
        }
        try {
            return objectMapper.writeValueAsString(payload);
        } catch (Exception e) {
            return String.valueOf(payload);
        }
    }
}
