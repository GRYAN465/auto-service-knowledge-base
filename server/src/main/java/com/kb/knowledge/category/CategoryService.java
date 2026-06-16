package com.kb.knowledge.category;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.knowledge.category.dto.CategoryRequest;
import com.kb.knowledge.category.dto.CategoryTreeVO;
import com.kb.knowledge.category.entity.KbCategory;
import com.kb.knowledge.category.mapper.KbCategoryMapper;
import org.springframework.stereotype.Service;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Deque;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

@Service
public class CategoryService {

    private final KbCategoryMapper categoryMapper;

    public CategoryService(KbCategoryMapper categoryMapper) {
        this.categoryMapper = categoryMapper;
    }

    public List<CategoryTreeVO> tree() {
        List<KbCategory> categories = categoryMapper.selectList(Wrappers.<KbCategory>lambdaQuery()
                .orderByAsc(KbCategory::getSort)
                .orderByAsc(KbCategory::getId));
        return buildTree(categories);
    }

    public KbCategory get(Long id) {
        KbCategory category = categoryMapper.selectById(id);
        if (category == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "分类不存在");
        }
        return category;
    }

    public KbCategory create(CategoryRequest request) {
        KbCategory category = new KbCategory();
        apply(category, request);
        categoryMapper.insert(category);
        return category;
    }

    public KbCategory update(Long id, CategoryRequest request) {
        KbCategory category = get(id);
        if (id.equals(request.getParentId())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "上级分类不能是自身");
        }
        apply(category, request);
        categoryMapper.updateById(category);
        return get(id);
    }

    public void delete(Long id) {
        get(id);
        // 级联删除：连同全部子孙分类一并逻辑删除（客户端已做「输入确定删除」二次确认）。
        // 知识文章对分类的占用校验留待模块 4（届时校验 kb_article.category_id）。
        List<KbCategory> all = categoryMapper.selectList(Wrappers.<KbCategory>lambdaQuery());
        for (Long subId : collectSubtreeIds(id, all)) {
            categoryMapper.deleteById(subId);
        }
    }

    /** 收集 rootId 自身及其所有子孙的 id（逐层展开）。 */
    private static List<Long> collectSubtreeIds(Long rootId, List<KbCategory> all) {
        Map<Long, List<Long>> childrenOf = new HashMap<>();
        for (KbCategory c : all) {
            childrenOf.computeIfAbsent(c.getParentId(), k -> new ArrayList<>()).add(c.getId());
        }
        List<Long> ids = new ArrayList<>();
        Deque<Long> stack = new ArrayDeque<>();
        stack.push(rootId);
        while (!stack.isEmpty()) {
            Long cur = stack.pop();
            ids.add(cur);
            List<Long> kids = childrenOf.get(cur);
            if (kids != null) {
                for (Long kid : kids) {
                    stack.push(kid);
                }
            }
        }
        return ids;
    }

    private static void apply(KbCategory category, CategoryRequest request) {
        category.setParentId(request.getParentId() == null ? 0L : request.getParentId());
        category.setName(request.getName());
        category.setCode(request.getCode());
        category.setSort(request.getSort() == null ? 0 : request.getSort());
        category.setStatus(request.getStatus() == null ? "ENABLED" : request.getStatus());
    }

    private static List<CategoryTreeVO> buildTree(List<KbCategory> categories) {
        Map<Long, CategoryTreeVO> nodes = new LinkedHashMap<>();
        Map<Long, Long> parentOf = new LinkedHashMap<>();
        categories.stream()
                .sorted(Comparator.comparing(KbCategory::getSort, Comparator.nullsLast(Integer::compareTo))
                        .thenComparing(KbCategory::getId))
                .forEach(category -> {
                    nodes.put(category.getId(), new CategoryTreeVO(category.getId(), category.getParentId(),
                            category.getName(), category.getCode(), category.getSort(), category.getStatus(),
                            new ArrayList<>()));
                    parentOf.put(category.getId(), category.getParentId());
                });
        List<CategoryTreeVO> roots = new ArrayList<>();
        for (Map.Entry<Long, CategoryTreeVO> e : nodes.entrySet()) {
            CategoryTreeVO parent = nodes.get(parentOf.get(e.getKey()));
            if (parent == null) {
                roots.add(e.getValue());
            } else {
                parent.getChildren().add(e.getValue());
            }
        }
        return roots;
    }
}
