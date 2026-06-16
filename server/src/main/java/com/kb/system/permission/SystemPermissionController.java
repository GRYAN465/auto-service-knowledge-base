package com.kb.system.permission;

import com.kb.common.Result;
import com.kb.system.permission.dto.PermissionRequest;
import com.kb.system.permission.dto.PermissionTreeVO;
import com.kb.system.permission.entity.SysPermission;
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
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@Tag(name = "系统管理-菜单按钮权限")
@RestController
@RequestMapping("/system/permission")
public class SystemPermissionController {

    private final SystemPermissionAdminService permissionService;

    public SystemPermissionController(SystemPermissionAdminService permissionService) {
        this.permissionService = permissionService;
    }

    @Operation(summary = "权限树")
    @GetMapping({ "", "/tree" })
    @PreAuthorize("hasAuthority('system:permission:list')")
    public Result<List<PermissionTreeVO>> tree() {
        return Result.ok(permissionService.tree());
    }

    @Operation(summary = "权限详情")
    @GetMapping("/{id}")
    @PreAuthorize("hasAuthority('system:permission:list')")
    public Result<SysPermission> get(@PathVariable Long id) {
        return Result.ok(permissionService.get(id));
    }

    @Operation(summary = "新增权限")
    @PostMapping
    @PreAuthorize("hasAuthority('system:permission:create')")
    public Result<SysPermission> create(@RequestBody @Valid PermissionRequest request) {
        return Result.ok(permissionService.create(request));
    }

    @Operation(summary = "编辑权限")
    @PutMapping("/{id}")
    @PreAuthorize("hasAuthority('system:permission:update')")
    public Result<SysPermission> update(@PathVariable Long id, @RequestBody @Valid PermissionRequest request) {
        return Result.ok(permissionService.update(id, request));
    }

    @Operation(summary = "删除权限")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('system:permission:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        permissionService.delete(id);
        return Result.ok();
    }
}
