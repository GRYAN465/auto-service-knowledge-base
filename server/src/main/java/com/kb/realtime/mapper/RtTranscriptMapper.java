package com.kb.realtime.mapper;

import com.baomidou.mybatisplus.core.mapper.BaseMapper;
import com.kb.realtime.entity.RtTranscript;

/**
 * rt_transcript Mapper。转写片段写入与按 sessionId+seqNo 排序读取。
 */
public interface RtTranscriptMapper extends BaseMapper<RtTranscript> {
}