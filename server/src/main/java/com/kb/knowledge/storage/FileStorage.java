package com.kb.knowledge.storage;

import org.springframework.core.io.Resource;

/**
 * 文件存储抽象。一期落本地（{@link LocalFileStorage}），后续可换 MinIO/OSS 而不改业务层。
 */
public interface FileStorage {

    /**
     * 存储文件，返回可用于 {@link #load} 的相对路径（如 {@code 2026/06/uuid.pdf}）。
     */
    String store(byte[] bytes, String originalName);

    /** 按 {@link #store} 返回的路径读取文件资源。 */
    Resource load(String path);

    /** 删除文件（不存在时静默）。 */
    void delete(String path);
}
