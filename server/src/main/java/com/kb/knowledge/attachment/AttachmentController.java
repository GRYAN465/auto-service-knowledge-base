package com.kb.knowledge.attachment;

import com.kb.common.Result;
import com.kb.knowledge.article.entity.KbAttachment;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.core.io.Resource;
import org.springframework.http.HttpHeaders;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.security.access.prepost.PreAuthorize;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.multipart.MultipartFile;

import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.List;

@Tag(name = "知识管理-附件")
@RestController
@RequestMapping("/knowledge/attachment")
public class AttachmentController {

    private final AttachmentService attachmentService;

    public AttachmentController(AttachmentService attachmentService) {
        this.attachmentService = attachmentService;
    }

    @Operation(summary = "上传附件")
    @PostMapping("/upload")
    @PreAuthorize("hasAuthority('knowledge:attachment:upload')")
    public Result<KbAttachment> upload(@RequestParam Long articleId, @RequestParam("file") MultipartFile file) {
        return Result.ok(attachmentService.upload(articleId, file));
    }

    @Operation(summary = "附件列表")
    @GetMapping
    @PreAuthorize("hasAuthority('knowledge:article:list')")
    public Result<List<KbAttachment>> list(@RequestParam Long articleId) {
        return Result.ok(attachmentService.list(articleId));
    }

    @Operation(summary = "下载附件")
    @GetMapping("/{id}/download")
    @PreAuthorize("hasAuthority('knowledge:article:list')")
    public ResponseEntity<Resource> download(@PathVariable Long id) {
        KbAttachment attachment = attachmentService.get(id);
        Resource resource = attachmentService.loadForDownload(attachment);
        String fileName = attachment.getFileName() == null ? ("attachment-" + id) : attachment.getFileName();
        String encoded = URLEncoder.encode(fileName, StandardCharsets.UTF_8).replace("+", "%20");
        return ResponseEntity.ok()
                .header(HttpHeaders.CONTENT_DISPOSITION,
                        "attachment; filename=\"" + encoded + "\"; filename*=UTF-8''" + encoded)
                .contentType(MediaType.APPLICATION_OCTET_STREAM)
                .body(resource);
    }

    @Operation(summary = "删除附件")
    @DeleteMapping("/{id}")
    @PreAuthorize("hasAuthority('knowledge:attachment:delete')")
    public Result<Void> delete(@PathVariable Long id) {
        attachmentService.delete(id);
        return Result.ok();
    }
}
