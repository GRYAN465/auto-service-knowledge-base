package com.kb.openapi;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.openapi.dto.OpenAppRequest;
import com.kb.openapi.dto.OpenAppVO;
import com.kb.openapi.dto.ResetSecretResponse;
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

@Tag(name = "开放能力-应用管理")
@RestController
@RequestMapping("/openapi/app")
public class OpenAppController {

    private final OpenAppService appService;

    public OpenAppController(OpenAppService appService) {
        this.appService = appService;
    }

    @Operation(summary = "开放应用分页")
    @GetMapping
    @PreAuthorize("hasAuthority('openapi:app:list')")
    public Result<PageResult<OpenAppVO>> page(@RequestParam(defaultValue = "1") long page,
                                              @RequestParam(defaultValue = "20") long pageSize,
                                              @RequestParam(required = false) String keyword,
                                              @RequestParam(required = false) String status) {
        return Result.ok(appService.page(page, pageSize, keyword, status));
    }

    @Operation(summary = "开放应用详情")
    @GetMapping("/{id}")
    @PreAuthorize("hasAuthority('openapi:app:list')")
    public Result<OpenAppVO> get(@PathVariable Long id) {
        return Result.ok(appService.get(id));
    }

    @Operation(summary = "新增开放应用")
    @PostMapping
    @PreAuthorize("hasAuthority('openapi:app:create')")
    public Result<OpenAppVO> create(@RequestBody @Valid OpenAppRequest request) {
        return Result.ok(appService.create(request));
    }

    @Operation(summary = "编辑开放应用")
    @PutMapping("/{id}")
    @PreAuthorize("hasAuthority('openapi:app:update')")
    public Result<OpenAppVO> update(@PathVariable Long id, @RequestBody @Valid OpenAppRequest request) {
        return Result.ok(appService.update(id, request));
    }

    @Operation(summary = "重置 AppSecret")
    @PutMapping("/{id}/secret")
    @PreAuthorize("hasAuthority('openapi:app:update')")
    public Result<ResetSecretResponse> resetSecret(@PathVariable Long id) {
        return Result.ok(new ResetSecretResponse(appService.resetSecret(id)));
    }

    @Operation(summary = "删除开放应用")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('openapi:app:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        appService.delete(id);
        return Result.ok();
    }
}
