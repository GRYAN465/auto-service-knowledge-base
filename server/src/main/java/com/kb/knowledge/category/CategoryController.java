package com.kb.knowledge.category;

import com.kb.common.Result;
import com.kb.knowledge.category.dto.CategoryRequest;
import com.kb.knowledge.category.dto.CategoryTreeVO;
import com.kb.knowledge.category.entity.KbCategory;
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

@Tag(name = "知识基础-分类")
@RestController
@RequestMapping("/knowledge/category")
public class CategoryController {

    private final CategoryService categoryService;

    public CategoryController(CategoryService categoryService) {
        this.categoryService = categoryService;
    }

    @Operation(summary = "分类树")
    @GetMapping({ "", "/tree" })
    @PreAuthorize("hasAuthority('knowledge:category:list')")
    public Result<List<CategoryTreeVO>> tree() {
        return Result.ok(categoryService.tree());
    }

    @Operation(summary = "分类详情")
    @GetMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:category:list')")
    public Result<KbCategory> get(@PathVariable Long id) {
        return Result.ok(categoryService.get(id));
    }

    @Operation(summary = "新增分类")
    @PostMapping
    @PreAuthorize("hasAuthority('knowledge:category:create')")
    public Result<KbCategory> create(@RequestBody @Valid CategoryRequest request) {
        return Result.ok(categoryService.create(request));
    }

    @Operation(summary = "编辑分类")
    @PutMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:category:update')")
    public Result<KbCategory> update(@PathVariable Long id, @RequestBody @Valid CategoryRequest request) {
        return Result.ok(categoryService.update(id, request));
    }

    @Operation(summary = "删除分类")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:category:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        categoryService.delete(id);
        return Result.ok();
    }
}
