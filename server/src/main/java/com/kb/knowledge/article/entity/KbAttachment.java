package com.kb.knowledge.article.entity;

import com.baomidou.mybatisplus.annotation.TableName;
import com.kb.common.BaseEntity;
import lombok.Data;
import lombok.EqualsAndHashCode;

/**
 * 知识附件（kb_attachment）。文件落本地 FileStorage，path 存相对路径。
 */
@EqualsAndHashCode(callSuper = true)
@Data
@TableName("kb_attachment")
public class KbAttachment extends BaseEntity {

    private Long articleId;
    private String fileName;
    private String filePath;
    /** PDF/WORD/PPT/AUDIO/VIDEO/OTHER */
    private String fileType;
    private Long fileSize;
    /** LOCAL（后续 MINIO） */
    private String storageType;
    private Long downloadCount;
}
