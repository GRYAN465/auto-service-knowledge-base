package com.kb.openapi;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.BusinessException;
import com.kb.common.PageResult;
import com.kb.common.ResultCode;
import com.kb.openapi.dto.OpenAppRequest;
import com.kb.openapi.dto.OpenAppVO;
import com.kb.openapi.entity.OpenApp;
import com.kb.openapi.mapper.OpenAppMapper;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.security.SecureRandom;
import java.util.HexFormat;
import java.util.List;
import java.util.UUID;

@Service
public class OpenAppService {

    private static final SecureRandom RANDOM = new SecureRandom();

    private final OpenAppMapper appMapper;

    public OpenAppService(OpenAppMapper appMapper) {
        this.appMapper = appMapper;
    }

    public PageResult<OpenAppVO> page(long page, long pageSize, String keyword, String status) {
        IPage<OpenApp> data = appMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<OpenApp>lambdaQuery()
                        .and(StringUtils.hasText(keyword), q -> q
                                .like(OpenApp::getAppName, keyword)
                                .or()
                                .like(OpenApp::getAppKey, keyword))
                        .eq(StringUtils.hasText(status), OpenApp::getStatus, status)
                        .orderByDesc(OpenApp::getId));
        List<OpenAppVO> list = data.getRecords().stream().map(OpenAppVO::from).toList();
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(), list);
    }

    public OpenAppVO get(Long id) {
        return OpenAppVO.from(require(id));
    }

    public OpenAppVO create(OpenAppRequest request) {
        String appKey = StringUtils.hasText(request.getAppKey()) ? request.getAppKey().trim() : generateAppKey();
        ensureAppKeyUnique(appKey, null);
        OpenApp app = new OpenApp();
        apply(app, request);
        app.setAppKey(appKey);
        app.setAppSecret(StringUtils.hasText(request.getAppSecret()) ? request.getAppSecret().trim() : generateSecret());
        appMapper.insert(app);
        OpenAppVO vo = OpenAppVO.from(app);
        vo.setAppSecret(app.getAppSecret());
        return vo;
    }

    public OpenAppVO update(Long id, OpenAppRequest request) {
        OpenApp app = require(id);
        String appKey = StringUtils.hasText(request.getAppKey()) ? request.getAppKey().trim() : app.getAppKey();
        ensureAppKeyUnique(appKey, id);
        apply(app, request);
        app.setAppKey(appKey);
        if (StringUtils.hasText(request.getAppSecret())) {
            app.setAppSecret(request.getAppSecret().trim());
        }
        appMapper.updateById(app);
        return get(id);
    }

    public String resetSecret(Long id) {
        OpenApp app = require(id);
        String secret = generateSecret();
        app.setAppSecret(secret);
        appMapper.updateById(app);
        return secret;
    }

    public void delete(Long id) {
        require(id);
        appMapper.deleteById(id);
    }

    private OpenApp require(Long id) {
        OpenApp app = appMapper.selectById(id);
        if (app == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "开放应用不存在");
        }
        return app;
    }

    private void ensureAppKeyUnique(String appKey, Long excludeId) {
        Long count = appMapper.selectCount(Wrappers.<OpenApp>lambdaQuery()
                .eq(OpenApp::getAppKey, appKey)
                .ne(excludeId != null, OpenApp::getId, excludeId));
        if (count > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "AppKey 已存在");
        }
    }

    private static void apply(OpenApp app, OpenAppRequest request) {
        app.setAppName(request.getAppName());
        app.setStatus(StringUtils.hasText(request.getStatus()) ? request.getStatus() : "ENABLED");
        app.setRateLimit(request.getRateLimit() == null ? 60 : Math.max(1, request.getRateLimit()));
        app.setScope(StringUtils.hasText(request.getScope()) ? request.getScope().trim() : "search,detail");
        app.setRemark(request.getRemark());
    }

    private static String generateAppKey() {
        return "ak_" + UUID.randomUUID().toString().replace("-", "");
    }

    private static String generateSecret() {
        byte[] bytes = new byte[24];
        RANDOM.nextBytes(bytes);
        return "sk_" + HexFormat.of().formatHex(bytes);
    }
}
