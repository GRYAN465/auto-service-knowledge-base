package com.kb.common;

import lombok.Getter;

/**
 * 业务异常：由 {@link GlobalExceptionHandler} 统一捕获并转成 {@link Result#fail}。
 */
@Getter
public class BusinessException extends RuntimeException {

    private final int code;

    public BusinessException(String message) {
        this(ResultCode.PARAM_ERROR, message);
    }

    public BusinessException(int code, String message) {
        super(message);
        this.code = code;
    }
}
