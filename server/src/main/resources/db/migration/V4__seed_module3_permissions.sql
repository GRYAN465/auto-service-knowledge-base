-- =============================================================================
-- V4 模块 3 知识基础（分类 / 标签）补充按钮权限码
-- 父节点 303「分类标签管理」(route=category, knowledge:category:list) 已在 V2 种下；
-- 3031「标签管理」(knowledge:tag:list) 亦已存在。此处仅补 6 个增删改按钮并幂等授予 ADMIN。
-- 不回改 V2/V3，避免已执行环境发生 Flyway checksum 变化。
-- =============================================================================

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3032, 303, '新增分类', 'BUTTON', 'knowledge:category:create', NULL, NULL, 2, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3032);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3033, 303, '编辑分类', 'BUTTON', 'knowledge:category:update', NULL, NULL, 3, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3033);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3034, 303, '删除分类', 'BUTTON', 'knowledge:category:delete', NULL, NULL, 4, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3034);

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3035, 303, '新增标签', 'BUTTON', 'knowledge:tag:create', NULL, NULL, 5, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3035);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3036, 303, '编辑标签', 'BUTTON', 'knowledge:tag:update', NULL, NULL, 6, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3036);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3037, 303, '删除标签', 'BUTTON', 'knowledge:tag:delete', NULL, NULL, 7, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3037);

-- ADMIN(role 1) 幂等补授新增按钮 -------------------------------------------
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, p.id
FROM sys_permission p
WHERE NOT EXISTS (
    SELECT 1 FROM sys_role_permission rp
    WHERE rp.role_id = 1 AND rp.permission_id = p.id
);
