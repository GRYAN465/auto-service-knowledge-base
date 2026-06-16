package com.kb.system.auth;

import com.kb.system.auth.dto.MenuVO;
import com.kb.system.permission.entity.SysPermission;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * 把用户被授予的权限节点装配成客户端导航菜单树：
 * 只取 DIR/MENU（BUTTON 仅作权限码），按 parent_id 嵌套、保持 sort 顺序，剪掉无子菜单的空目录。
 *
 * <p>MenuVO 的 {@code name} 取权限的 {@code route}（客户端 PageRouter 注册名），{@code title} 取权限名。
 * DIR 无 route → name 为空，客户端按「有子节点」识别为分组。
 */
final class MenuTreeBuilder {

    private MenuTreeBuilder() {
    }

    static List<MenuVO> build(List<SysPermission> granted) {
        // 1. 建节点（仅 DIR/MENU），入序即 sort 序（mapper 已 ORDER BY sort）
        Map<Long, MenuVO> nodes = new LinkedHashMap<>();
        Map<Long, Long> parentOf = new LinkedHashMap<>();
        for (SysPermission p : granted) {
            if (!"DIR".equals(p.getType()) && !"MENU".equals(p.getType())) {
                continue;
            }
            nodes.put(p.getId(), new MenuVO(p.getRoute(), p.getName(), p.getIcon(), new ArrayList<>()));
            parentOf.put(p.getId(), p.getParentId());
        }
        // 2. 连父子；父节点未授予时该节点上提为根（容错）
        List<MenuVO> roots = new ArrayList<>();
        for (Map.Entry<Long, MenuVO> e : nodes.entrySet()) {
            Long parentId = parentOf.get(e.getKey());
            MenuVO parent = parentId == null ? null : nodes.get(parentId);
            if (parent != null) {
                parent.getChildren().add(e.getValue());
            } else {
                roots.add(e.getValue());
            }
        }
        // 3. 剪掉没有任何可点菜单的空目录
        roots.removeIf(MenuTreeBuilder::pruneEmptyDir);
        return roots;
    }

    /** 递归剪枝，返回 true 表示该节点本身是「无 route 且无子节点」的空目录，应被移除。 */
    private static boolean pruneEmptyDir(MenuVO node) {
        node.getChildren().removeIf(MenuTreeBuilder::pruneEmptyDir);
        boolean hasRoute = node.getName() != null && !node.getName().isBlank();
        return !hasRoute && node.getChildren().isEmpty();
    }
}
