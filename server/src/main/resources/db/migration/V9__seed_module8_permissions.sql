-- =============================================================================
-- V9 模块 8 知识能力开放 API：开放应用管理按钮权限
-- 菜单 402「API 开放管理」与 openapi:app:list 已在 V2 种入。
-- =============================================================================

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 4021, 402, '新增开放应用', 'BUTTON', 'openapi:app:create', NULL, NULL, 1, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 4021);

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 4022, 402, '编辑开放应用', 'BUTTON', 'openapi:app:update', NULL, NULL, 2, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 4022);

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 4023, 402, '删除开放应用', 'BUTTON', 'openapi:app:delete', NULL, NULL, 3, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 4023);

INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, p.id
FROM sys_permission p
WHERE p.id IN (400, 402, 4021, 4022, 4023)
  AND NOT EXISTS (
      SELECT 1 FROM sys_role_permission rp
      WHERE rp.role_id = 1 AND rp.permission_id = p.id
  );
