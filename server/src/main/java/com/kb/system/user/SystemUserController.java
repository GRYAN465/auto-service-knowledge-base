package com.kb.system.user;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.system.user.dto.PasswordRequest;
import com.kb.system.user.dto.StatusRequest;
import com.kb.system.user.dto.UserRequest;
import com.kb.system.user.dto.UserVO;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.PutMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@Tag(name = "系统管理-人员")
@RestController
@RequestMapping("/system/user")
public class SystemUserController {

    private final SystemUserService userService;

    public SystemUserController(SystemUserService userService) {
        this.userService = userService;
    }

    @Operation(summary = "人员分页")
    @GetMapping
    @PreAuthorize("hasAuthority('system:user:list')")
    public Result<PageResult<UserVO>> page(@RequestParam(defaultValue = "1") long page,
                                           @RequestParam(defaultValue = "20") long pageSize,
                                           @RequestParam(required = false) String keyword,
                                           @RequestParam(required = false) Long orgId,
                                           @RequestParam(required = false) String status) {
        return Result.ok(userService.page(page, pageSize, keyword, orgId, status));
    }

    @Operation(summary = "人员详情")
    @GetMapping("/{id}")
    @PreAuthorize("hasAuthority('system:user:list')")
    public Result<UserVO> get(@PathVariable Long id) {
        return Result.ok(userService.get(id));
    }

    @Operation(summary = "新增人员")
    @PostMapping
    @PreAuthorize("hasAuthority('system:user:create')")
    public Result<UserVO> create(@RequestBody @Valid UserRequest request) {
        return Result.ok(userService.create(request));
    }

    @Operation(summary = "编辑人员")
    @PutMapping("/{id}")
    @PreAuthorize("hasAuthority('system:user:update')")
    public Result<UserVO> update(@PathVariable Long id, @RequestBody @Valid UserRequest request) {
        return Result.ok(userService.update(id, request));
    }

    @Operation(summary = "改密")
    @PutMapping("/{id}/password")
    @PreAuthorize("hasAuthority('system:user:update')")
    public Result<Void> updatePassword(@PathVariable Long id, @RequestBody @Valid PasswordRequest request) {
        userService.updatePassword(id, request.getPassword());
        return Result.ok();
    }

    @Operation(summary = "启停")
    @PutMapping("/{id}/status")
    @PreAuthorize("hasAuthority('system:user:update')")
    public Result<Void> updateStatus(@PathVariable Long id, @RequestBody @Valid StatusRequest request) {
        userService.updateStatus(id, request.getStatus());
        return Result.ok();
    }

    @Operation(summary = "删除人员")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('system:user:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        userService.delete(id);
        return Result.ok();
    }
}
