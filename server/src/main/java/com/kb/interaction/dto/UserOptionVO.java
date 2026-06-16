package com.kb.interaction.dto;

import lombok.Data;

/** 分享收件人选项（启用用户的 id + 姓名），供坐席选人，避开 system:user:list 权限。 */
@Data
public class UserOptionVO {
    private Long id;
    private String realName;

    public UserOptionVO() {
    }

    public UserOptionVO(Long id, String realName) {
        this.id = id;
        this.realName = realName;
    }
}
