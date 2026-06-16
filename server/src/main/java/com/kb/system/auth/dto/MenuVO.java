package com.kb.system.auth.dto;

import lombok.AllArgsConstructor;
import lombok.Data;

import java.util.List;

/**
 * 导航菜单节点。{@code name} 对应客户端 PageRouter 注册名（见《API契约.md》§2、页面树 §14.1）。
 */
@Data
@AllArgsConstructor
public class MenuVO {

    private String name;
    private String title;
    private String icon;
    private List<MenuVO> children;

    public static MenuVO of(String name, String title, String icon) {
        return new MenuVO(name, title, icon, List.of());
    }
}
