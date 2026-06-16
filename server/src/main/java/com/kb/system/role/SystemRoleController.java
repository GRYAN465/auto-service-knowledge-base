package com.kb.system.role;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.system.role.dto.AssignPermissionsRequest;
import com.kb.system.role.dto.RoleRequest;
import com.kb.system.role.dto.RoleVO;
import com.kb.system.role.entity.SysRole;
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

import java.util.List;

@Tag(name = "系统管理-角色")
@RestController
@RequestMapping("/system/role")
public class SystemRoleController {

    private final SystemRoleService roleService;

    public SystemRoleController(SystemRoleService roleService) {
        this.roleService = roleService;
    }

    @Operation(summary = "角色分页")
    @GetMapping
    @PreAuthorize("hasAuthority('system:role:list')")
    public Result<PageResult<RoleVO>> page(@RequestParam(defaultValue = "1") long page,
                                           @RequestParam(defaultValue = "20") long pageSize,
                                           @RequestParam(required = false) String keyword,
                                           @RequestParam(required = false) String status) {
        return Result.ok(roleService.page(page, pageSize, keyword, status));
    }

    @Operation(summary = "启用角色选项")
    @GetMapping("/options")
    @PreAuthorize("hasAuthority('system:role:list') or hasAuthority('system:user:list')")
    public Result<List<SysRole>> options() {
        return Result.ok(roleService.options());
    }

    @Operation(summary = "角色详情")
    @GetMapping("/{id}")
    @PreAuthorize("hasAuthority('system:role:list')")
    public Result<RoleVO> get(@PathVariable Long id) {
        return Result.ok(roleService.get(id));
    }

    @Operation(summary = "新增角色")
    @PostMapping
    @PreAuthorize("hasAuthority('system:role:create')")
    public Result<RoleVO> create(@RequestBody @Valid RoleRequest request) {
        return Result.ok(roleService.create(request));
    }

    @Operation(summary = "编辑角色")
    @PutMapping("/{id}")
    @PreAuthorize("hasAuthority('system:role:update')")
    public Result<RoleVO> update(@PathVariable Long id, @RequestBody @Valid RoleRequest request) {
        return Result.ok(roleService.update(id, request));
    }

    @Operation(summary = "分配权限")
    @PutMapping("/{id}/permissions")
    @PreAuthorize("hasAuthority('system:role:permission')")
    public Result<Void> assignPermissions(@PathVariable Long id,
                                          @RequestBody AssignPermissionsRequest request) {
        roleService.assignPermissions(id, request.getPermissionIds());
        return Result.ok();
    }

    @Operation(summary = "删除角色")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('system:role:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        roleService.delete(id);
        return Result.ok();
    }
}
