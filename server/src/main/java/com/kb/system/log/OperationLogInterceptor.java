package com.kb.system.log;

import com.kb.security.LoginUser;
import com.kb.security.SecurityUtils;
import com.kb.system.log.entity.SysOperationLog;
import com.kb.system.log.mapper.SysOperationLogMapper;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;
import org.springframework.stereotype.Component;
import org.springframework.web.servlet.HandlerInterceptor;

import java.time.LocalDateTime;
import java.util.Locale;

@Component
public class OperationLogInterceptor implements HandlerInterceptor {

    private static final String START_ATTR = OperationLogInterceptor.class.getName() + ".START";

    private final SysOperationLogMapper operationLogMapper;

    public OperationLogInterceptor(SysOperationLogMapper operationLogMapper) {
        this.operationLogMapper = operationLogMapper;
    }

    @Override
    public boolean preHandle(HttpServletRequest request, HttpServletResponse response, Object handler) {
        request.setAttribute(START_ATTR, System.currentTimeMillis());
        return true;
    }

    @Override
    public void afterCompletion(HttpServletRequest request, HttpServletResponse response, Object handler,
                                Exception ex) {
        if (!shouldRecord(request)) {
            return;
        }
        Long start = (Long) request.getAttribute(START_ATTR);
        long cost = start == null ? 0L : System.currentTimeMillis() - start;
        LoginUser user = SecurityUtils.getLoginUserOrNull();

        SysOperationLog log = new SysOperationLog();
        log.setUserId(user == null ? null : user.getUserId());
        log.setUsername(user == null ? null : user.getUsername());
        log.setModule(moduleOf(request.getRequestURI()));
        log.setOperation(operationOf(request.getMethod(), request.getRequestURI()));
        log.setMethod(request.getMethod());
        log.setRequestUri(request.getRequestURI());
        log.setRequestParam(request.getQueryString());
        log.setStatus(ex == null && response.getStatus() < 400 ? "SUCCESS" : "FAIL");
        log.setErrorMsg(ex == null ? null : ex.getMessage());
        log.setCostTime(cost);
        log.setOperationTime(LocalDateTime.now());
        operationLogMapper.insert(log);
    }

    private static boolean shouldRecord(HttpServletRequest request) {
        String method = request.getMethod().toUpperCase(Locale.ROOT);
        return "POST".equals(method) || "PUT".equals(method) || "DELETE".equals(method);
    }

    private static String moduleOf(String uri) {
        if (uri.contains("/system/org")) {
            return "机构管理";
        }
        if (uri.contains("/system/user")) {
            return "人员管理";
        }
        if (uri.contains("/system/role")) {
            return "角色管理";
        }
        if (uri.contains("/system/permission")) {
            return "权限管理";
        }
        if (uri.contains("/knowledge/category")) {
            return "知识分类";
        }
        if (uri.contains("/knowledge/tag")) {
            return "知识标签";
        }
        if (uri.contains("/knowledge/article")) {
            return "知识管理";
        }
        if (uri.contains("/knowledge/attachment")) {
            return "知识附件";
        }
        return "系统管理";
    }

    private static String operationOf(String method, String uri) {
        String upper = method.toUpperCase(Locale.ROOT);
        if ("POST".equals(upper) && uri.endsWith("/submit")) {
            return "提交审核";
        }
        if ("POST".equals(upper) && uri.endsWith("/audit")) {
            return "审核";
        }
        if ("POST".equals(upper) && uri.endsWith("/online")) {
            return "上线";
        }
        if ("POST".equals(upper) && uri.endsWith("/offline")) {
            return "下线";
        }
        if ("POST".equals(upper) && uri.endsWith("/upload")) {
            return "上传附件";
        }
        if ("POST".equals(upper)) {
            return "新增";
        }
        if ("PUT".equals(upper) && uri.endsWith("/permissions")) {
            return "分配权限";
        }
        if ("PUT".equals(upper) && uri.endsWith("/password")) {
            return "改密";
        }
        if ("PUT".equals(upper) && uri.endsWith("/status")) {
            return "启停";
        }
        if ("PUT".equals(upper)) {
            return "编辑";
        }
        if ("DELETE".equals(upper)) {
            return "删除";
        }
        return upper;
    }
}
