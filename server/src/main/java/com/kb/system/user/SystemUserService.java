package com.kb.system.user;

import com.baomidou.mybatisplus.core.metadata.IPage;
import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.baomidou.mybatisplus.extension.plugins.pagination.Page;
import com.kb.common.BusinessException;
import com.kb.common.PageResult;
import com.kb.common.ResultCode;
import com.kb.system.user.dto.UserRequest;
import com.kb.system.user.dto.UserVO;
import com.kb.system.user.entity.SysUser;
import com.kb.system.user.entity.SysUserRole;
import com.kb.system.user.mapper.SysUserMapper;
import com.kb.system.user.mapper.SysUserRoleMapper;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.stereotype.Service;
import org.springframework.util.StringUtils;

import java.util.List;

@Service
public class SystemUserService {

    private static final String DEFAULT_PASSWORD = "123456";

    private final SysUserMapper userMapper;
    private final SysUserRoleMapper userRoleMapper;
    private final PasswordEncoder passwordEncoder;

    public SystemUserService(SysUserMapper userMapper, SysUserRoleMapper userRoleMapper,
                             PasswordEncoder passwordEncoder) {
        this.userMapper = userMapper;
        this.userRoleMapper = userRoleMapper;
        this.passwordEncoder = passwordEncoder;
    }

    public PageResult<UserVO> page(long page, long pageSize, String keyword, Long orgId, String status) {
        IPage<SysUser> data = userMapper.selectPage(new Page<>(page, pageSize),
                Wrappers.<SysUser>lambdaQuery()
                        .and(StringUtils.hasText(keyword), q -> q
                                .like(SysUser::getUsername, keyword)
                                .or()
                                .like(SysUser::getRealName, keyword)
                                .or()
                                .like(SysUser::getPhone, keyword))
                        .eq(orgId != null, SysUser::getOrgId, orgId)
                        .eq(StringUtils.hasText(status), SysUser::getStatus, status)
                        .orderByDesc(SysUser::getId));
        List<UserVO> records = data.getRecords().stream()
                .map(user -> UserVO.from(user, userRoleMapper.selectRoleIdsByUserId(user.getId())))
                .toList();
        return new PageResult<>(data.getTotal(), data.getCurrent(), data.getSize(), records);
    }

    public UserVO get(Long id) {
        SysUser user = userMapper.selectById(id);
        if (user == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户不存在");
        }
        return UserVO.from(user, userRoleMapper.selectRoleIdsByUserId(id));
    }

    public UserVO create(UserRequest request) {
        ensureUsernameUnique(request.getUsername(), null);
        SysUser user = new SysUser();
        apply(user, request);
        String rawPassword = StringUtils.hasText(request.getPassword()) ? request.getPassword() : DEFAULT_PASSWORD;
        user.setPassword(passwordEncoder.encode(rawPassword));
        userMapper.insert(user);
        replaceRoles(user.getId(), request.getRoleIds());
        return get(user.getId());
    }

    public UserVO update(Long id, UserRequest request) {
        SysUser user = userMapper.selectById(id);
        if (user == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户不存在");
        }
        ensureUsernameUnique(request.getUsername(), id);
        apply(user, request);
        userMapper.updateById(user);
        if (request.getRoleIds() != null) {
            replaceRoles(id, request.getRoleIds());
        }
        return get(id);
    }

    public void updatePassword(Long id, String password) {
        SysUser user = userMapper.selectById(id);
        if (user == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户不存在");
        }
        user.setPassword(passwordEncoder.encode(password));
        userMapper.updateById(user);
    }

    public void updateStatus(Long id, String status) {
        SysUser user = userMapper.selectById(id);
        if (user == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户不存在");
        }
        if (id == 1L && !"ENABLED".equals(status)) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "内置管理员不能禁用");
        }
        user.setStatus(status);
        userMapper.updateById(user);
    }

    public void delete(Long id) {
        if (id == 1L) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "内置管理员不能删除");
        }
        get(id);
        userRoleMapper.delete(Wrappers.<SysUserRole>lambdaQuery().eq(SysUserRole::getUserId, id));
        userMapper.deleteById(id);
    }

    private void ensureUsernameUnique(String username, Long excludeId) {
        Long count = userMapper.selectCount(Wrappers.<SysUser>lambdaQuery()
                .eq(SysUser::getUsername, username)
                .ne(excludeId != null, SysUser::getId, excludeId));
        if (count > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户名已存在");
        }
    }

    private void replaceRoles(Long userId, List<Long> roleIds) {
        userRoleMapper.delete(Wrappers.<SysUserRole>lambdaQuery().eq(SysUserRole::getUserId, userId));
        if (roleIds == null || roleIds.isEmpty()) {
            return;
        }
        for (Long roleId : roleIds.stream().distinct().toList()) {
            SysUserRole relation = new SysUserRole();
            relation.setUserId(userId);
            relation.setRoleId(roleId);
            userRoleMapper.insert(relation);
        }
    }

    private static void apply(SysUser user, UserRequest request) {
        user.setOrgId(request.getOrgId());
        user.setUsername(request.getUsername());
        user.setRealName(request.getRealName());
        user.setNickname(request.getNickname());
        user.setAvatar(request.getAvatar());
        user.setEmail(request.getEmail());
        user.setPhone(request.getPhone());
        user.setGender(request.getGender() == null ? "U" : request.getGender());
        user.setStatus(request.getStatus() == null ? "ENABLED" : request.getStatus());
        user.setExpireTime(request.getExpireTime());
        user.setRemark(request.getRemark());
    }
}
