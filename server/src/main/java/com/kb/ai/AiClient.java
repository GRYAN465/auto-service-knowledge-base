package com.kb.ai;

import com.kb.ai.dto.AiIndexRequest;
import com.kb.ai.dto.AiIndexResponse;
import com.kb.ai.dto.AiLlmConfigPush;
import com.kb.ai.dto.AiLlmTestRequest;
import com.kb.ai.dto.AiLlmTestResponse;
import com.kb.ai.dto.AiQaRequest;
import com.kb.ai.dto.AiQaResponse;
import com.kb.ai.dto.AiRecommendMatchRequest;
import com.kb.ai.dto.AiRecommendMatchResponse;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.MediaType;
import org.springframework.http.client.SimpleClientHttpRequestFactory;
import org.springframework.http.converter.StringHttpMessageConverter;
import org.springframework.stereotype.Component;
import org.springframework.web.client.RestClient;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.Map;

/**
 * 调用 Python ai-service（FastAPI）的出站 HTTP 客户端：向量化入库 / 移除 / 检索增强问答 / 首页推荐匹配。
 * 用 Spring Boot 自带 {@link RestClient}（无需额外依赖）；读超时放宽以容 LLM 延迟。
 *
 * <p>请求/响应 DTO 用 {@code @JsonProperty} 显式映射 snake_case，独立于本项目 Jackson 默认（camelCase），
 * 故无论 RestClient 用哪个 ObjectMapper 都正确。出错（连接失败 / 4xx / 5xx）抛出 RestClient 的运行时异常，
 * 由调用方（{@code VectorIndexService}/{@code QaService}）决定吞或抛——索引同步吞、问答上抛。
 */
@Component
public class AiClient {

    private final RestClient restClient;

    public AiClient(@Value("${kb.ai.base-url:http://localhost:8000}") String baseUrl,
                    @Value("${kb.ai.connect-timeout-ms:3000}") int connectTimeoutMs,
                    @Value("${kb.ai.read-timeout-ms:60000}") int readTimeoutMs) {
        SimpleClientHttpRequestFactory factory = new SimpleClientHttpRequestFactory();
        factory.setConnectTimeout(connectTimeoutMs);
        factory.setReadTimeout(readTimeoutMs);
        this.restClient = RestClient.builder()
                .baseUrl(baseUrl)
                .requestFactory(factory)
                .messageConverters(converters -> {
                    converters.add(0, new StringHttpMessageConverter(StandardCharsets.UTF_8));
                })
                .build();
    }

    /** 向量化并入库（Python 侧 upsert=先删后写，整篇覆盖）。 */
    public AiIndexResponse index(Long articleId, String knowledgeType, List<String> texts) {
        return restClient.post()
                .uri("/ai/index")
                .contentType(MediaType.APPLICATION_JSON)
                .body(new AiIndexRequest(articleId, knowledgeType, texts))
                .retrieve()
                .body(AiIndexResponse.class);
    }

    /** 移除某知识的全部向量块（下线/删除时）。 */
    public void removeIndex(Long articleId, String knowledgeType) {
        restClient.post()
                .uri("/ai/index/remove")
                .contentType(MediaType.APPLICATION_JSON)
                .body(Map.of("article_id", articleId, "knowledge_type", knowledgeType != null ? knowledgeType : ""))
                .retrieve()
                .toBodilessEntity();
    }

    /** 检索增强问答。{@code allowedArticleIds=null} 表示不限（全体在线）。 */
    public AiQaResponse qa(String question, String knowledgeType, Integer topK, List<Long> allowedArticleIds) {
        return restClient.post()
                .uri("/ai/qa")
                .contentType(MediaType.APPLICATION_JSON)
                .body(new AiQaRequest(question, knowledgeType, topK, allowedArticleIds))
                .retrieve()
                .body(AiQaResponse.class);
    }

    /**
     * 下发 LLM 运行时配置（M9.4）：Python 改 settings + 落盘 + 重置单例，立即生效无需重启。
     * 停用时三者传空串。连接失败/4xx/5xx 抛出运行时异常，由调用方做 best-effort 处理。
     */
    public void pushLlmConfig(String baseUrl, String apiKey, String model, Double temperature, Integer maxTokens) {
        restClient.post()
                .uri("/ai/llm/config")
                .contentType(MediaType.APPLICATION_JSON)
                .body(new AiLlmConfigPush(baseUrl, apiKey, model, temperature, maxTokens))
                .retrieve()
                .toBodilessEntity();
    }

    /**
     * 用候选值测试 LLM 连通性（M9.4）：Python 建临时客户端做一次极短生成，不落库不改单例。
     * 连通失败时 Python 仍返回 200（ok=false）；仅 Python 本身不可用才抛异常。
     */
    public AiLlmTestResponse testLlmConfig(String baseUrl, String apiKey, String model,
                                           Double temperature, Integer maxTokens) {
        return restClient.post()
                .uri("/ai/llm/test")
                .contentType(MediaType.APPLICATION_JSON)
                .body(new AiLlmTestRequest(baseUrl, apiKey, model, temperature, maxTokens))
                .retrieve()
                .body(AiLlmTestResponse.class);
    }

    /** 首页推荐：用户画像 vs 标签/文章向量相似度。 */
    public AiRecommendMatchResponse recommendMatch(AiRecommendMatchRequest request) {
        return restClient.post()
                .uri("/ai/recommend/match")
                .contentType(MediaType.APPLICATION_JSON)
                .body(request)
                .retrieve()
                .body(AiRecommendMatchResponse.class);
    }
}
