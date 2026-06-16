package com.kb.knowledge.attachment;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.knowledge.article.entity.KbAttachment;
import com.kb.knowledge.article.mapper.KbAttachmentMapper;
import com.kb.knowledge.storage.FileStorage;
import org.springframework.core.io.Resource;
import org.springframework.stereotype.Service;
import org.springframework.web.multipart.MultipartFile;

import java.io.IOException;
import java.util.List;
import java.util.Locale;

/**
 * 知识附件业务：上传落 {@link FileStorage} + 入 kb_attachment；下载累加计数；删除同时清理文件。
 */
@Service
public class AttachmentService {

    private final KbAttachmentMapper attachmentMapper;
    private final FileStorage fileStorage;

    public AttachmentService(KbAttachmentMapper attachmentMapper, FileStorage fileStorage) {
        this.attachmentMapper = attachmentMapper;
        this.fileStorage = fileStorage;
    }

    public KbAttachment upload(Long articleId, MultipartFile file) {
        if (articleId == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "缺少 articleId");
        }
        if (file == null || file.isEmpty()) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "文件为空");
        }
        String originalName = file.getOriginalFilename();
        byte[] bytes;
        try {
            bytes = file.getBytes();
        } catch (IOException e) {
            throw new BusinessException(ResultCode.SERVER_ERROR, "读取上传文件失败：" + e.getMessage());
        }
        String path = fileStorage.store(bytes, originalName);

        KbAttachment attachment = new KbAttachment();
        attachment.setArticleId(articleId);
        attachment.setFileName(originalName);
        attachment.setFilePath(path);
        attachment.setFileType(fileTypeOf(originalName));
        attachment.setFileSize(file.getSize());
        attachment.setStorageType("LOCAL");
        attachment.setDownloadCount(0L);
        attachmentMapper.insert(attachment);
        return attachment;
    }

    public List<KbAttachment> list(Long articleId) {
        return attachmentMapper.selectList(Wrappers.<KbAttachment>lambdaQuery()
                .eq(KbAttachment::getArticleId, articleId)
                .orderByAsc(KbAttachment::getId));
    }

    public KbAttachment get(Long id) {
        KbAttachment attachment = attachmentMapper.selectById(id);
        if (attachment == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "附件不存在");
        }
        return attachment;
    }

    /** 取下载资源并累加下载计数。 */
    public Resource loadForDownload(KbAttachment attachment) {
        Resource resource = fileStorage.load(attachment.getFilePath());
        KbAttachment update = new KbAttachment();
        update.setId(attachment.getId());
        update.setDownloadCount((attachment.getDownloadCount() == null ? 0L : attachment.getDownloadCount()) + 1);
        attachmentMapper.updateById(update);
        return resource;
    }

    public void delete(Long id) {
        KbAttachment attachment = get(id);
        attachmentMapper.deleteById(id);
        fileStorage.delete(attachment.getFilePath());
    }

    private static String fileTypeOf(String name) {
        if (name == null) {
            return "OTHER";
        }
        String lower = name.toLowerCase(Locale.ROOT);
        if (lower.endsWith(".pdf")) {
            return "PDF";
        }
        if (lower.endsWith(".doc") || lower.endsWith(".docx")) {
            return "WORD";
        }
        if (lower.endsWith(".ppt") || lower.endsWith(".pptx")) {
            return "PPT";
        }
        if (lower.endsWith(".mp3") || lower.endsWith(".wav") || lower.endsWith(".m4a")) {
            return "AUDIO";
        }
        if (lower.endsWith(".mp4") || lower.endsWith(".mov") || lower.endsWith(".avi")) {
            return "VIDEO";
        }
        return "OTHER";
    }
}
