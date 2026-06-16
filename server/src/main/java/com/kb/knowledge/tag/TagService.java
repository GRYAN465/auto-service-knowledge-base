package com.kb.knowledge.tag;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.BusinessException;
import com.kb.common.PageResult;
import com.kb.common.ResultCode;
import com.kb.knowledge.tag.dto.TagRequest;
import com.kb.knowledge.tag.entity.KbTag;
import com.kb.knowledge.tag.mapper.KbTagMapper;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

@Service
public class TagService {

    private final KbTagMapper tagMapper;

    public TagService(KbTagMapper tagMapper) {
        this.tagMapper = tagMapper;
    }

    public PageResult<KbTag> page(long page, long pageSize, String keyword) {
        IPage<KbTag> data = tagMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<KbTag>lambdaQuery()
                        .like(StringUtils.hasText(keyword), KbTag::getName, keyword)
                        .orderByAsc(KbTag::getSort)
                        .orderByDesc(KbTag::getId));
        return PageResult.of(data);
    }

    public KbTag get(Long id) {
        KbTag tag = tagMapper.selectById(id);
        if (tag == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "标签不存在");
        }
        return tag;
    }

    public KbTag create(TagRequest request) {
        ensureNameUnique(request.getName(), null);
        KbTag tag = new KbTag();
        apply(tag, request);
        tagMapper.insert(tag);
        return tag;
    }

    public KbTag update(Long id, TagRequest request) {
        KbTag tag = get(id);
        ensureNameUnique(request.getName(), id);
        apply(tag, request);
        tagMapper.updateById(tag);
        return get(id);
    }

    public void delete(Long id) {
        get(id);
        // 文章对标签的占用（kb_article_tag）校验留待模块 4。
        tagMapper.deleteById(id);
    }

    private void ensureNameUnique(String name, Long excludeId) {
        Long count = tagMapper.selectCount(Wrappers.<KbTag>lambdaQuery()
                .eq(KbTag::getName, name)
                .ne(excludeId != null, KbTag::getId, excludeId));
        if (count > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "标签名称已存在");
        }
    }

    private static void apply(KbTag tag, TagRequest request) {
        tag.setName(request.getName());
        tag.setColor(request.getColor());
        tag.setSort(request.getSort() == null ? 0 : request.getSort());
    }
}
