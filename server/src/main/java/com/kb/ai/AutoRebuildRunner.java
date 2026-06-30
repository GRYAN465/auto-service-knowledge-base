package com.kb.ai;

import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.context.event.ApplicationReadyEvent;
import org.springframework.context.event.EventListener;
import org.springframework.stereotype.Component;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.Map;
import java.util.concurrent.CompletableFuture;

/**
 * 启动自动重建向量索引：Python 启动时会清空 chroma，本组件在应用就绪后等待 Python 就绪，
 * 然后调用 {@code reindexAllOnline()} 把全部在线知识重建进向量库。
 *
 * <p>设计要点：
 * <ul>
 *   <li>先等 Python 健康检查通过（最多 {@code maxWaitMs}），避免 Python 还没起好就发起重建。</li>
 *   <li>若首次等待失败，90 秒后再自动重试一次（常见：Java 先于 Python 启动）。</li>
 *   <li>重建本身是 best-effort：失败的文章记 FAILED 但不阻断启动，单篇异常已在
 *       {@link VectorIndexService#reindexArticle} 内吞掉。</li>
 *   <li>可通过 {@code kb.ai.auto-rebuild-on-startup=false} 关闭本行为（prod 集群多实例场景）。</li>
 * </ul>
 */
@Slf4j
@Component
public class AutoRebuildRunner {

    private final VectorIndexService vectorIndexService;
    private final String pythonBaseUrl;
    private final int maxWaitMs;
    private final boolean enabled;

    public AutoRebuildRunner(VectorIndexService vectorIndexService,
                             @Value("${kb.ai.base-url:http://localhost:8000}") String pythonBaseUrl,
                             @Value("${kb.ai.auto-rebuild-on-startup:true}") boolean enabled,
                             @Value("${kb.ai.auto-rebuild-wait-ms:120000}") int maxWaitMs) {
        this.vectorIndexService = vectorIndexService;
        this.pythonBaseUrl = pythonBaseUrl;
        this.enabled = enabled;
        this.maxWaitMs = maxWaitMs;
    }

    @EventListener(ApplicationReadyEvent.class)
    public void onReady() {
        // 后台执行：避免在 ApplicationReady 线程上同步跑全量 reindex（可能数分钟），
        // 与首页推荐/问答等并发打 Python 导致接口长时间无响应或连接被重置。
        CompletableFuture.runAsync(() -> attemptRebuild("启动", true));
    }

    private void attemptRebuild(String phase, boolean scheduleRetry) {
        if (!enabled) {
            log.info("auto-rebuild-on-startup=false，跳过启动重建");
            return;
        }
        if (!waitForPython()) {
            log.warn("{}：Python ai-service 在 {}ms 内未就绪。"
                            + " 请先启动 ai-service:8000，或稍后 POST /api/ai/index/rebuild 手动重建向量。",
                    phase, maxWaitMs);
            if (scheduleRetry) {
                CompletableFuture.runAsync(() -> {
                    try {
                        Thread.sleep(90_000);
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                        return;
                    }
                    attemptRebuild("延迟重试", false);
                });
            }
            return;
        }
        logHealthSnapshot();
        try {
            Map<String, Integer> result = vectorIndexService.reindexAllOnline();
            log.info("{}自动重建索引完成：{}", phase, result);
        } catch (Exception e) {
            log.warn("{}自动重建索引异常（已忽略）：{}", phase, e.toString());
        }
    }

    /** 轮询 Python /health，直到 UP 或超时。*/
    private boolean waitForPython() {
        HttpClient client = HttpClient.newBuilder()
                .connectTimeout(Duration.ofSeconds(5))
                .build();
        long deadline = System.currentTimeMillis() + maxWaitMs;
        int attempt = 0;
        while (System.currentTimeMillis() < deadline) {
            attempt++;
            try {
                HttpRequest req = HttpRequest.newBuilder()
                        .uri(URI.create(pythonBaseUrl + "/health"))
                        .timeout(Duration.ofSeconds(5))
                        .GET()
                        .build();
                HttpResponse<String> resp = client.send(req, HttpResponse.BodyHandlers.ofString());
                if (resp.statusCode() == 200 && resp.body().contains("UP")) {
                    log.info("Python ai-service 就绪（第{}次探测）", attempt);
                    return true;
                }
            } catch (Exception ignored) {
                // 连接被拒 / 超时等，继续等
            }
            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                return false;
            }
        }
        return false;
    }

    private void logHealthSnapshot() {
        try {
            HttpClient client = HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(3)).build();
            HttpRequest req = HttpRequest.newBuilder()
                    .uri(URI.create(pythonBaseUrl + "/health"))
                    .timeout(Duration.ofSeconds(5))
                    .GET()
                    .build();
            HttpResponse<String> resp = client.send(req, HttpResponse.BodyHandlers.ofString());
            log.info("Python /health 快照：{}", resp.body());
        } catch (Exception e) {
            log.debug("读取 Python /health 快照失败：{}", e.toString());
        }
    }
}
