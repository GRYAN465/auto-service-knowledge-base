package com.kb.system.org;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.system.org.dto.OrgRequest;
import com.kb.system.org.dto.OrgTreeVO;
import com.kb.system.org.entity.SysOrg;
import com.kb.system.org.mapper.SysOrgMapper;
import com.kb.system.user.entity.SysUser;
import com.kb.system.user.mapper.SysUserMapper;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

@Service
public class OrgService {

    private final SysOrgMapper orgMapper;
    private final SysUserMapper userMapper;

    public OrgService(SysOrgMapper orgMapper, SysUserMapper userMapper) {
        this.orgMapper = orgMapper;
        this.userMapper = userMapper;
    }

    public List<OrgTreeVO> tree() {
        List<SysOrg> orgs = orgMapper.selectList(Wrappers.<SysOrg>lambdaQuery()
                .orderByAsc(SysOrg::getSort)
                .orderByAsc(SysOrg::getId));
        return buildTree(orgs);
    }

    public SysOrg get(Long id) {
        SysOrg org = orgMapper.selectById(id);
        if (org == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "机构不存在");
        }
        return org;
    }

    public SysOrg create(OrgRequest request) {
        ensureCodeUnique(request.getCode(), null);
        SysOrg org = new SysOrg();
        apply(org, request);
        orgMapper.insert(org);
        return org;
    }

    public SysOrg update(Long id, OrgRequest request) {
        SysOrg org = get(id);
        if (id.equals(request.getParentId())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "上级机构不能是自身");
        }
        ensureCodeUnique(request.getCode(), id);
        apply(org, request);
        orgMapper.updateById(org);
        return get(id);
    }

    public void delete(Long id) {
        if (id == 1L) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "根机构不能删除");
        }
        get(id);
        Long childCount = orgMapper.selectCount(Wrappers.<SysOrg>lambdaQuery().eq(SysOrg::getParentId, id));
        if (childCount > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "存在下级机构，不能删除");
        }
        Long userCount = userMapper.selectCount(Wrappers.<SysUser>lambdaQuery().eq(SysUser::getOrgId, id));
        if (userCount > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "机构下存在人员，不能删除");
        }
        orgMapper.deleteById(id);
    }

    private void ensureCodeUnique(String code, Long excludeId) {
        Long count = orgMapper.selectCount(Wrappers.<SysOrg>lambdaQuery()
                .eq(SysOrg::getCode, code)
                .ne(excludeId != null, SysOrg::getId, excludeId));
        if (count > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "机构编码已存在");
        }
    }

    private static void apply(SysOrg org, OrgRequest request) {
        org.setParentId(request.getParentId() == null ? 0L : request.getParentId());
        org.setName(request.getName());
        org.setCode(request.getCode());
        org.setType(request.getType() == null ? "DEPT" : request.getType());
        org.setSort(request.getSort() == null ? 0 : request.getSort());
        org.setStatus(request.getStatus() == null ? "ENABLED" : request.getStatus());
    }

    private static List<OrgTreeVO> buildTree(List<SysOrg> orgs) {
        Map<Long, OrgTreeVO> nodes = new LinkedHashMap<>();
        Map<Long, Long> parentOf = new LinkedHashMap<>();
        orgs.stream()
                .sorted(Comparator.comparing(SysOrg::getSort, Comparator.nullsLast(Integer::compareTo))
                        .thenComparing(SysOrg::getId))
                .forEach(org -> {
                    nodes.put(org.getId(), new OrgTreeVO(org.getId(), org.getParentId(), org.getName(),
                            org.getCode(), org.getType(), org.getSort(), org.getStatus(), new ArrayList<>()));
                    parentOf.put(org.getId(), org.getParentId());
                });
        List<OrgTreeVO> roots = new ArrayList<>();
        for (Map.Entry<Long, OrgTreeVO> e : nodes.entrySet()) {
            OrgTreeVO parent = nodes.get(parentOf.get(e.getKey()));
            if (parent == null) {
                roots.add(e.getValue());
            } else {
                parent.getChildren().add(e.getValue());
            }
        }
        return roots;
    }
}
