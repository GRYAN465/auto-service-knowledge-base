package com.kb.knowledge.tag;

import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.knowledge.tag.dto.TagRequest;
import com.kb.knowledge.tag.entity.KbTag;
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

@Tag(name = "知识基础-标签")
@RestController
@RequestMapping("/knowledge/tag")
public class TagController {

    private final TagService tagService;

    public TagController(TagService tagService) {
        this.tagService = tagService;
    }

    @Operation(summary = "标签分页")
    @GetMapping
    @PreAuthorize("hasAuthority('knowledge:tag:list')")
    public Result<PageResult<KbTag>> page(@RequestParam(defaultValue = "1") long page,
                                          @RequestParam(defaultValue = "20") long pageSize,
                                          @RequestParam(required = false) String keyword) {
        return Result.ok(tagService.page(page, pageSize, keyword));
    }

    @Operation(summary = "标签下拉选项（社区常用标签，按 sort）")
    @GetMapping("/options")
    @PreAuthorize("hasAuthority('knowledge:search')")
    public Result<java.util.List<KbTag>> options() {
        return Result.ok(tagService.listOptions());
    }

    @Operation(summary = "标签详情")
    @GetMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:tag:list')")
    public Result<KbTag> get(@PathVariable Long id) {
        return Result.ok(tagService.get(id));
    }

    @Operation(summary = "新增标签")
    @PostMapping
    @PreAuthorize("hasAuthority('knowledge:tag:create')")
    public Result<KbTag> create(@RequestBody @Valid TagRequest request) {
        return Result.ok(tagService.create(request));
    }

    @Operation(summary = "编辑标签")
    @PutMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:tag:update')")
    public Result<KbTag> update(@PathVariable Long id, @RequestBody @Valid TagRequest request) {
        return Result.ok(tagService.update(id, request));
    }

    @Operation(summary = "删除标签")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:tag:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        tagService.delete(id);
        return Result.ok();
    }
}
