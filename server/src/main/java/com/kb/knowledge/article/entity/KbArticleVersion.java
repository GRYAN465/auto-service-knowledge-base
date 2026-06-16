package com.kb.knowledge.article.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

import java.time.LocalDateTime;

/**
 * 知识版本快照（kb_article_version）。不继承 BaseEntity：无逻辑删除、无 update 审计，仅留存档。
 */
@Data
@TableName("kb_article_version")
public class KbArticleVersion {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Integer versionNo;
    private String title;
    private String content;
    private Long editorId;
    private String changeLog;
    private LocalDateTime createTime;
}
