-- =============================================================================
-- V6 模块 6 知识互动 补充按钮权限码
-- interaction:favorite(菜单 202)/interaction:share(菜单 203)/interaction:comment(按钮 2011)
-- 已在 V2 种下且 ADMIN 已授；此处仅幂等补 interaction:like（点赞/点踩按钮，挂菜单 201
-- 知识检索下，id=2012），并幂等授予 ADMIN。
-- 不回改 V1-V5，避免已执行环境发生 Flyway checksum 变化。
-- 演示数据仍由独立脚本 db/demo/demo_data.sql 提供，不进迁移链。
-- =============================================================================

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 2012, 201, '点赞/点踩', 'BUTTON', 'interaction:like', NULL, NULL, 2, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 2012);

-- ADMIN(role 1) 幂等补授新增按钮 -------------------------------------------
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, p.id
FROM sys_permission p
WHERE NOT EXISTS (
    SELECT 1 FROM sys_role_permission rp
    WHERE rp.role_id = 1 AND rp.permission_id = p.id
);
