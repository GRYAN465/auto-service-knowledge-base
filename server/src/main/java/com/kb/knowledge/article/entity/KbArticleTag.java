package com.kb.knowledge.article.entity;

import com.baomidou.mybatisplus.annotation.IdType;
import com.baomidou.mybatisplus.annotation.TableId;
import com.baomidou.mybatisplus.annotation.TableName;
import lombok.Data;

/**
 * 知识-标签关系（kb_article_tag）。纯关系表，无逻辑删除，删文章/改标签时整行增删。
 */
@Data
@TableName("kb_article_tag")
public class KbArticleTag {

    @TableId(type = IdType.AUTO)
    private Long id;

    private Long articleId;
    private Long tagId;
}
