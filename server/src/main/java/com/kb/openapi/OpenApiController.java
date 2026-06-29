package com.kb.openapi;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.kb.ai.QaService;
import com.kb.ai.dto.QaVO;
import com.kb.common.BusinessException;
import com.kb.common.PageResult;
import com.kb.common.Result;
import com.kb.knowledge.article.ArticleService;
import com.kb.knowledge.article.dto.ArticleDetailVO;
import com.kb.knowledge.search.dto.SearchArticleVO;
import com.kb.knowledge.search.SearchService;
import com.kb.openapi.dto.OpenQaRequest;
import com.kb.openapi.entity.OpenApp;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.validation.Valid;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

@Tag(name = "开放能力-对外 API")
@RestController
@RequestMapping("/open/v1")
public class OpenApiController {

    private final OpenApiAuthService authService;
    private final SearchService searchService;
    private final ArticleService articleService;
    private final ObjectMapper objectMapper;
    private final QaService qaService;

    public OpenApiController(OpenApiAuthService authService, SearchService searchService,
                             ArticleService articleService, ObjectMapper objectMapper,
                             QaService qaService) {
        this.authService = authService;
        this.searchService = searchService;
        this.articleService = articleService;
        this.objectMapper = objectMapper;
        this.qaService = qaService;
    }

    @Operation(summary = "对外知识检索")
    @GetMapping("/search")
    public Result<PageResult<SearchArticleVO>> search(@RequestParam(defaultValue = "1") long page,
                                                        @RequestParam(defaultValue = "20") long pageSize,
                                                        @RequestParam(required = false) String keyword,
                                                        @RequestParam(required = false) Long categoryId,
                                                        @RequestParam(required = false) Long tagId,
                                                        HttpServletRequest request) {
        long start = System.currentTimeMillis();
        OpenApp app = null;
        int code = 0;
        try {
            app = authService.authenticate(request, "search", "");
            return Result.ok(searchService.search(page, pageSize, keyword, categoryId, tagId, null, null,
                    authService.clientIp(request)));
        } catch (BusinessException e) {
            code = e.getCode();
            throw e;
        } finally {
            authService.record(app, request.getHeader("X-App-Key"), request, request.getQueryString(), code,
                    System.currentTimeMillis() - start);
        }
    }

    @Operation(summary = "对外知识详情")
    @GetMapping("/article/{id}")
    public Result<ArticleDetailVO> detail(@PathVariable Long id, HttpServletRequest request) {
        long start = System.currentTimeMillis();
        OpenApp app = null;
        int code = 0;
        try {
            app = authService.authenticate(request, "detail", "");
            ArticleDetailVO detail = articleService.detail(id);
            if (!"ONLINE".equals(detail.getStatus())) {
                throw new BusinessException(1001, "知识未上线或不可访问");
            }
            return Result.ok(detail);
        } catch (BusinessException e) {
            code = e.getCode();
            throw e;
        } finally {
            authService.record(app, request.getHeader("X-App-Key"), request, request.getQueryString(), code,
                    System.currentTimeMillis() - start);
        }
    }

    @Operation(summary = "对外问答（检索增强）")
    @PostMapping("/qa")
    public Result<QaVO> qa(@RequestBody String bodyText, HttpServletRequest request) {
        long start = System.currentTimeMillis();
        OpenApp app = null;
        int code = 0;
        try {
            app = authService.authenticate(request, "qa", bodyText);
            OpenQaRequest body = readQa(bodyText);
            validateQa(body);
            return Result.ok(qaService.openQa(body.getQuestion(), body.getTopK()));
        } catch (BusinessException e) {
            code = e.getCode();
            throw e;
        } finally {
            authService.record(app, request.getHeader("X-App-Key"), request, bodyText, code,
                    System.currentTimeMillis() - start);
        }
    }

    private static void validateQa(@Valid OpenQaRequest body) {
        if (body == null || body.getQuestion() == null || body.getQuestion().isBlank()) {
            throw new BusinessException(1001, "问题不能为空");
        }
    }

    private OpenQaRequest readQa(String bodyText) {
        try {
            return objectMapper.readValue(bodyText, OpenQaRequest.class);
        } catch (JsonProcessingException e) {
            throw new BusinessException(1001, "请求体格式错误");
        }
    }
}
