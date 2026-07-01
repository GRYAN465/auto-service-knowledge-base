package com.kb.realtime.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.kb.realtime.entity.RtSession;

/**
 * rt_active_session Mapper。会话生命周期 + 定时推荐扫描走此接口。
 */
public interface RtSessionMapper extends BaseMapper<RtSession> {
}