-- =============================================================================
-- V3 模块 2 系统管理补充权限码
-- 不回改 V2，避免已执行环境发生 Flyway checksum 变化。
-- =============================================================================

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6014, 601, '机构查询', 'BUTTON', 'system:org:list', NULL, NULL, 4, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6014);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6015, 601, '新增机构', 'BUTTON', 'system:org:create', NULL, NULL, 5, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6015);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6016, 601, '编辑机构', 'BUTTON', 'system:org:update', NULL, NULL, 6, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6016);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6017, 601, '删除机构', 'BUTTON', 'system:org:delete', NULL, NULL, 7, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6017);

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6021, 602, '新增角色', 'BUTTON', 'system:role:create', NULL, NULL, 1, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6021);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6022, 602, '编辑角色', 'BUTTON', 'system:role:update', NULL, NULL, 2, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6022);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6023, 602, '删除角色', 'BUTTON', 'system:role:delete', NULL, NULL, 3, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6023);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6024, 602, '分配权限', 'BUTTON', 'system:role:permission', NULL, NULL, 4, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6024);

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6031, 603, '新增权限', 'BUTTON', 'system:permission:create', NULL, NULL, 1, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6031);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6032, 603, '编辑权限', 'BUTTON', 'system:permission:update', NULL, NULL, 2, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6032);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 6033, 603, '删除权限', 'BUTTON', 'system:permission:delete', NULL, NULL, 3, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 6033);

INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, p.id
FROM sys_permission p
WHERE NOT EXISTS (
    SELECT 1 FROM sys_role_permission rp
    WHERE rp.role_id = 1 AND rp.permission_id = p.id
);
