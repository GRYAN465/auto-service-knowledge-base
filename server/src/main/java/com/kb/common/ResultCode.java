package com.kb.common;

/**
 * 业务错误码约定（见《API契约.md》§1）。
 * 0 成功；1xxx 参数/校验；2xxx 鉴权与权限；3xxx 系统管理；4xxx 知识；5xxx 应用/互动；6xxx 开放 API。
 */
public final class ResultCode {

    private ResultCode() {
    }

    public static final int SUCCESS = 0;

    public static final int PARAM_ERROR = 1001;

    public static final int UNAUTHORIZED = 2001;
    public static final int TOKEN_INVALID = 2002;
    public static final int FORBIDDEN = 2003;

    public static final int SERVER_ERROR = 5000;
}
