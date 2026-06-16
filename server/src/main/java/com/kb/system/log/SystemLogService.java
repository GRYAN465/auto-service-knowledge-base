package com.kb.system.log;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.PageResult;
import com.kb.system.log.entity.SysLoginLog;
import com.kb.system.log.entity.SysOperationLog;
import com.kb.system.log.mapper.SysLoginLogMapper;
import com.kb.system.log.mapper.SysOperationLogMapper;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

@Service
public class SystemLogService {

    private final SysLoginLogMapper loginLogMapper;
    private final SysOperationLogMapper operationLogMapper;

    public SystemLogService(SysLoginLogMapper loginLogMapper, SysOperationLogMapper operationLogMapper) {
        this.loginLogMapper = loginLogMapper;
        this.operationLogMapper = operationLogMapper;
    }

    public PageResult<SysLoginLog> loginPage(long page, long pageSize, String username, String status) {
        IPage<SysLoginLog> data = loginLogMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<SysLoginLog>lambdaQuery()
                        .like(StringUtils.hasText(username), SysLoginLog::getUsername, username)
                        .eq(StringUtils.hasText(status), SysLoginLog::getStatus, status)
                        .orderByDesc(SysLoginLog::getLoginTime)
                        .orderByDesc(SysLoginLog::getId));
        return PageResult.of(data);
    }

    public PageResult<SysOperationLog> operationPage(long page, long pageSize, String username, String module,
                                                     String status) {
        IPage<SysOperationLog> data = operationLogMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<SysOperationLog>lambdaQuery()
                        .like(StringUtils.hasText(username), SysOperationLog::getUsername, username)
                        .like(StringUtils.hasText(module), SysOperationLog::getModule, module)
                        .eq(StringUtils.hasText(status), SysOperationLog::getStatus, status)
                        .orderByDesc(SysOperationLog::getOperationTime)
                        .orderByDesc(SysOperationLog::getId));
        return PageResult.of(data);
    }
}
