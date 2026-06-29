package com.kb.system.auth;

import com.baomidou.mybatisplus.core.toolkit.Wrappers;
import com.kb.common.BusinessException;
import com.kb.common.ResultCode;
import com.kb.security.JwtTokenProvider;
import com.kb.system.auth.dto.MeResponse;
import com.kb.system.auth.dto.RegisterRequest;
import com.kb.system.auth.dto.TokenResponse;
import com.kb.system.org.OrgService;
import com.kb.system.role.entity.SysRole;
import com.kb.system.user.entity.SysUserRole;
import com.kb.system.user.mapper.SysUserRoleMapper;
import com.kb.system.auth.dto.UserVO;
import com.kb.system.log.entity.SysLoginLog;
import com.kb.system.log.mapper.SysLoginLogMapper;
import com.kb.system.permission.PermissionService;
import com.kb.system.permission.entity.SysPermission;
import com.kb.system.role.mapper.SysRoleMapper;
import com.kb.system.user.entity.SysUser;
import com.kb.system.user.mapper.SysUserMapper;
import org.springframework.security.crypto.password.PasswordEncoder;
import org.springframework.util.StringUtils;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;
import java.util.List;

/**
 * 鉴权业务：登录校验（DB + BCrypt）、装配当前用户响应（用户 + 角色 + 权限码 + 菜单树）。
 */
@Service
public class AuthService {

    private final SysUserMapper userMapper;
    private final SysRoleMapper roleMapper;
    private final PermissionService permissionService;
    private final PasswordEncoder passwordEncoder;
    private final JwtTokenProvider tokenProvider;
    private final SysLoginLogMapper loginLogMapper;
    private final SysUserRoleMapper userRoleMapper;
    private final OrgService orgService;

    public AuthService(SysUserMapper userMapper, SysRoleMapper roleMapper,
                       PermissionService permissionService, PasswordEncoder passwordEncoder,
                       JwtTokenProvider tokenProvider, SysLoginLogMapper loginLogMapper,
                       SysUserRoleMapper userRoleMapper, OrgService orgService) {
        this.userMapper = userMapper;
        this.roleMapper = roleMapper;
        this.permissionService = permissionService;
        this.passwordEncoder = passwordEncoder;
        this.tokenProvider = tokenProvider;
        this.loginLogMapper = loginLogMapper;
        this.userRoleMapper = userRoleMapper;
        this.orgService = orgService;
    }

    /** 登录：校验用户名/密码与账号状态，更新最近登录信息，签发 JWT。 */
    public TokenResponse login(String username, String rawPassword, String clientIp) {
        SysUser user = userMapper.selectOne(
                Wrappers.<SysUser>lambdaQuery().eq(SysUser::getUsername, username));
        // 用户不存在或密码错误统一提示，不泄露账号是否存在
        if (user == null || !passwordEncoder.matches(rawPassword, user.getPassword())) {
            recordLogin(null, username, clientIp, "FAIL", "用户名或密码错误");
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户名或密码错误");
        }
        try {
            assertUserAvailable(user);
        } catch (BusinessException e) {
            recordLogin(user.getId(), username, clientIp, "FAIL", e.getMessage());
            throw e;
        }
        userMapper.updateLoginInfo(user.getId(), LocalDateTime.now(), clientIp);
        recordLogin(user.getId(), username, clientIp, "SUCCESS", "登录成功");
        String token = tokenProvider.generate(user.getId(), user.getUsername());
        return new TokenResponse(token, tokenProvider.getExpireSeconds());
    }

    /** 当前用户 + 角色 + 权限码 + 菜单树（驱动客户端导航与按钮）。 */
    public MeResponse me(Long userId) {
        SysUser user = userMapper.selectById(userId);
        if (user == null) {
            throw new BusinessException(ResultCode.UNAUTHORIZED, "用户不存在或已被删除");
        }
        assertUserAvailable(user);
        UserVO userVO = new UserVO(user.getId(), user.getUsername(), user.getRealName(), user.getOrgId());
        List<String> roles = roleMapper.selectRoleCodesByUserId(userId);
        List<SysPermission> granted = permissionService.listGranted(userId);
        List<String> permissions = List.copyOf(permissionService.extractCodes(granted));
        return new MeResponse(userVO, roles, permissions, MenuTreeBuilder.build(granted));
    }

    /** 修改密码：校验旧密码后更新 BCrypt 密文。 */
    public void changePassword(Long userId, String oldPassword, String newPassword) {
        SysUser user = userMapper.selectById(userId);
        if (user == null) {
            throw new BusinessException(ResultCode.UNAUTHORIZED, "用户不存在");
        }
        if (!passwordEncoder.matches(oldPassword, user.getPassword())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "原密码不正确");
        }
        if (!StringUtils.hasText(newPassword) || newPassword.length() < 6) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "新密码至少 6 位");
        }
        user.setPassword(passwordEncoder.encode(newPassword));
        userMapper.updateById(user);
    }

    /**
     * 自助注册：创建启用账号并仅授予「普通用户 USER」角色。
     */
    @Transactional
    public void register(RegisterRequest request) {
        String username = request.getUsername().trim();
        ensureUsernameUnique(username, null);
        if (request.getOrgId() == null || request.getOrgId() <= 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "请选择所属机构");
        }
        var org = orgService.get(request.getOrgId());
        if (!"ENABLED".equals(org.getStatus())) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "所选机构不可用");
        }
        SysRole userRole = roleMapper.selectOne(Wrappers.<SysRole>lambdaQuery()
                .eq(SysRole::getCode, "USER")
                .eq(SysRole::getStatus, "ENABLED"));
        if (userRole == null) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "系统未配置普通用户角色");
        }

        SysUser user = new SysUser();
        user.setOrgId(request.getOrgId());
        user.setUsername(username);
        user.setPassword(passwordEncoder.encode(request.getPassword()));
        user.setRealName(StringUtils.hasText(request.getRealName()) ? request.getRealName().trim() : null);
        user.setNickname(StringUtils.hasText(request.getNickname()) ? request.getNickname().trim() : null);
        user.setEmail(StringUtils.hasText(request.getEmail()) ? request.getEmail().trim() : null);
        user.setPhone(StringUtils.hasText(request.getPhone()) ? request.getPhone().trim() : null);
        user.setGender(StringUtils.hasText(request.getGender()) ? request.getGender().trim() : "U");
        user.setStatus("ENABLED");
        userMapper.insert(user);

        SysUserRole relation = new SysUserRole();
        relation.setUserId(user.getId());
        relation.setRoleId(userRole.getId());
        userRoleMapper.insert(relation);
    }

    private void ensureUsernameUnique(String username, Long excludeId) {
        Long count = userMapper.selectCount(Wrappers.<SysUser>lambdaQuery()
                .eq(SysUser::getUsername, username)
                .ne(excludeId != null, SysUser::getId, excludeId));
        if (count > 0) {
            throw new BusinessException(ResultCode.PARAM_ERROR, "用户名已存在");
        }
    }

    private static void assertUserAvailable(SysUser user) {
        if (!"ENABLED".equals(user.getStatus())) {
            throw new BusinessException(ResultCode.FORBIDDEN, "账号已被禁用");
        }
        if (user.getExpireTime() != null && user.getExpireTime().isBefore(LocalDateTime.now())) {
            throw new BusinessException(ResultCode.FORBIDDEN, "账号已过期");
        }
    }

    private void recordLogin(Long userId, String username, String clientIp, String status, String msg) {
        SysLoginLog log = new SysLoginLog();
        log.setUserId(userId);
        log.setUsername(username);
        log.setLoginIp(clientIp);
        log.setClient("Qt/Desktop");
        log.setStatus(status);
        log.setMsg(msg);
        log.setLoginTime(LocalDateTime.now());
        loginLogMapper.insert(log);
    }
}
