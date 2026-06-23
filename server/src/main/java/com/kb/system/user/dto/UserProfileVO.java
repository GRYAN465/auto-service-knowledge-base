package com.kb.system.user.dto;

import lombok.Data;

/** 用户资料（个人中心 / 他人主页）。 */
@Data
public class UserProfileVO {

    private Long id;
    private String username;
    private String realName;
    /** 已上线知识篇数。 */
    private Long onlineArticleCount;
    /** 是否当前登录用户本人。 */
    private Boolean self;
    /** 收藏隐私开关，仅 self=true 时返回。 */
    private Boolean favoritePrivate;
    /** 当前查看者能否看到收藏列表。 */
    private Boolean favoritesVisible;
}
