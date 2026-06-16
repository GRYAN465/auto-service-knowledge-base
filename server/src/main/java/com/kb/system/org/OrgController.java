package com.kb.system.org;

import com.kb.common.Result;
import com.kb.system.org.dto.OrgRequest;
import com.kb.system.org.dto.OrgTreeVO;
import com.kb.system.org.entity.SysOrg;
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

@Tag(name = "系统管理-机构部门")
@RestController
@RequestMapping("/system/org")
public class OrgController {

    private final OrgService orgService;

    public OrgController(OrgService orgService) {
        this.orgService = orgService;
    }

    @Operation(summary = "机构树")
    @GetMapping({ "", "/tree" })
    @PreAuthorize("hasAuthority('system:org:list') or hasAuthority('system:user:list')")
    public Result<List<OrgTreeVO>> tree() {
        return Result.ok(orgService.tree());
    }

    @Operation(summary = "机构详情")
    @GetMapping("/{id}")
    @PreAuthorize("hasAuthority('system:org:list') or hasAuthority('system:user:list')")
    public Result<SysOrg> get(@PathVariable Long id) {
        return Result.ok(orgService.get(id));
    }

    @Operation(summary = "新增机构")
    @PostMapping
    @PreAuthorize("hasAuthority('system:org:create')")
    public Result<SysOrg> create(@RequestBody @Valid OrgRequest request) {
        return Result.ok(orgService.create(request));
    }

    @Operation(summary = "编辑机构")
    @PutMapping("/{id}")
    @PreAuthorize("hasAuthority('system:org:update')")
    public Result<SysOrg> update(@PathVariable Long id, @RequestBody @Valid OrgRequest request) {
        return Result.ok(orgService.update(id, request));
    }

    @Operation(summary = "删除机构")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('system:org:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        orgService.delete(id);
        return Result.ok();
    }
}
