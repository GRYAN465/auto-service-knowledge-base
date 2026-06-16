package com.kb.system.log;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.system.log.entity.SysLoginLog;
import com.kb.system.log.entity.SysOperationLog;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@Tag(name = "系统管理-日志")
@RestController
@RequestMapping("/system/log")
public class SystemLogController {

    private final SystemLogService logService;

    public SystemLogController(SystemLogService logService) {
        this.logService = logService;
    }

    @Operation(summary = "登录日志")
    @GetMapping("/login")
    @PreAuthorize("hasAuthority('system:log:login')")
    public Result<PageResult<SysLoginLog>> loginPage(@RequestParam(defaultValue = "1") long page,
                                                     @RequestParam(defaultValue = "20") long pageSize,
                                                     @RequestParam(required = false) String username,
                                                     @RequestParam(required = false) String status) {
        return Result.ok(logService.loginPage(page, pageSize, username, status));
    }

    @Operation(summary = "操作日志")
    @GetMapping("/operation")
    @PreAuthorize("hasAuthority('system:log:operation')")
    public Result<PageResult<SysOperationLog>> operationPage(@RequestParam(defaultValue = "1") long page,
                                                             @RequestParam(defaultValue = "20") long pageSize,
                                                             @RequestParam(required = false) String username,
                                                             @RequestParam(required = false) String module,
                                                             @RequestParam(required = false) String status) {
        return Result.ok(logService.operationPage(page, pageSize, username, module, status));
    }
}
