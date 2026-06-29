-- =============================================================================
-- V12 知识社区 + 个人中心：收藏隐私字段、个人中心菜单、全员可见授权
-- =============================================================================

ALTER TABLE sys_user
  ADD COLUMN favorite_private TINYINT NOT NULL DEFAULT 0 COMMENT '收藏隐私 0公开 1仅自己可见'
  AFTER remark;

-- 个人中心菜单（知识应用下，无 permission_code，登录即可见）
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 205, 200, '个人中心', 'MENU', NULL, 'personal.center', 'user', 4, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 205);

-- ADMIN 同步新菜单
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, 205
WHERE NOT EXISTS (SELECT 1 FROM sys_role_permission WHERE role_id = 1 AND permission_id = 205);

-- 全部内置角色（1-5）授予个人中心菜单
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT r.id, 205 FROM sys_role r
WHERE r.id BETWEEN 1 AND 5
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = r.id AND rp.permission_id = 205);

-- 知识应用目录 200 确保 AGENT/USER 可见（V7 已授，此处幂等补全其他角色）
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT r.id, 200 FROM sys_role r
WHERE r.id BETWEEN 1 AND 5
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = r.id AND rp.permission_id = 200);
