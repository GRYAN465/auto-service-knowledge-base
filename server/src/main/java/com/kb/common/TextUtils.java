package com.kb.common;

import org.springframework.util.StringUtils;

/**
 * 纯文本处理：HTML 剥离与预览截取（知识社区信息流 contentPreview）。
 */
public final class TextUtils {

    private TextUtils() {
    }

    /** 去除 HTML 标签与常见实体，压缩空白。 */
    public static String stripHtml(String html) {
        if (!StringUtils.hasText(html)) {
            return "";
        }
        String text = html.replaceAll("<[^>]+>", " ")
                .replace("&nbsp;", " ")
                .replace("&amp;", "&")
                .replace("&lt;", "<")
                .replace("&gt;", ">")
                .replaceAll("\\s+", " ")
                .trim();
        return text;
    }

    /** 取正文预览：优先 content，否则 summary，默认 20 字。 */
    public static String contentPreview(String content, String summary, int maxLen) {
        String raw = StringUtils.hasText(content) ? content : summary;
        String text = stripHtml(raw);
        if (text.length() <= maxLen) {
            return text;
        }
        return text.substring(0, maxLen) + "…";
    }
}
