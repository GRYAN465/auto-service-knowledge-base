package com.kb.knowledge.storage;

import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.io.PathResource;
import org.springframework.core.io.Resource;
import org.springframework.stereotype.Component;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import java.util.UUID;

/**
 * 本地文件存储：落 {@code kb.storage.local-dir}（默认 ./uploads），按 {@code yyyy/MM/uuid.ext} 分目录。
 */
@Component
public class LocalFileStorage implements FileStorage {

    private static final DateTimeFormatter MONTH = DateTimeFormatter.ofPattern("yyyy/MM");

    private final Path baseDir;

    public LocalFileStorage(@Value("${kb.storage.local-dir:./uploads}") String localDir) {
        this.baseDir = Paths.get(localDir).toAbsolutePath().normalize();
    }

    @Override
    public String store(byte[] bytes, String originalName) {
        String relative = MONTH.format(LocalDate.now()) + "/" + UUID.randomUUID().toString().replace("-", "")
                + extensionOf(originalName);
        Path target = baseDir.resolve(relative).normalize();
        if (!target.startsWith(baseDir)) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "非法存储路径");
        }
        try {
            Files.createDirectories(target.getParent());
            Files.write(target, bytes);
        } catch (IOException e) {
            throw new BusinessException(ResultCode.SERVER_ERROR, "文件写入失败：" + e.getMessage());
        }
        return relative;
    }

    @Override
    public Resource load(String path) {
        Path target = baseDir.resolve(path).normalize();
        if (!target.startsWith(baseDir) || !Files.exists(target)) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "文件不存在");
        }
        return new PathResource(target);
    }

    @Override
    public void delete(String path) {
        if (path == null || path.isBlank()) {
            return;
        }
        Path target = baseDir.resolve(path).normalize();
        if (!target.startsWith(baseDir)) {
            return;
        }
        try {
            Files.deleteIfExists(target);
        } catch (IOException ignored) {
            // 删除失败不阻断业务（逻辑删除已生效）。
        }
    }

    private static String extensionOf(String name) {
        if (name == null) {
            return "";
        }
        int dot = name.lastIndexOf('.');
        return dot >= 0 ? name.substring(dot) : "";
    }
}
