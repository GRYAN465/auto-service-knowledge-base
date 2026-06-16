package com.kb.common;

import com.baomidou.mybatisplus.core.metadata.IPage;
import lombok.Data;

import java.util.List;

/**
 * 分页响应：{@code {total, page, pageSize, list}}（见《API契约.md》§1）。
 */
@Data
public class PageResult<T> {

    private long total;
    private long page;
    private long pageSize;
    private List<T> list;

    public PageResult() {
    }

    public PageResult(long total, long page, long pageSize, List<T> list) {
        this.total = total;
        this.page = page;
        this.pageSize = pageSize;
        this.list = list;
    }

    /** 从 MyBatis-Plus 的 {@link IPage} 转换。 */
    public static <T> PageResult<T> of(IPage<T> page) {
        return new PageResult<>(page.getTotal(), page.getCurrent(), page.getSize(), page.getRecords());
    }
}
