-- =============================================================================
-- V5 模块 4 知识管理后台 补充按钮权限码
-- 父节点 301「知识管理」(route=knowledge.manage, knowledge:article:list)、302「审核中心」
-- (route=audit, knowledge:article:audit) 已在 V2 种下；
-- create(3011)/update(3012)/online(3013) 亦已存在。此处幂等补 delete/submit/offline
-- 及附件 upload/delete 五个按钮，并幂等授予 ADMIN。
-- 不回改 V1-V4，避免已执行环境发生 Flyway checksum 变化。
-- 演示数据仍由独立脚本 db/demo/demo_data.sql 提供，不进迁移链。
-- =============================================================================

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3014, 301, '删除知识', 'BUTTON', 'knowledge:article:delete', NULL, NULL, 4, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3014);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3015, 301, '提交审核', 'BUTTON', 'knowledge:article:submit', NULL, NULL, 5, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3015);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3016, 301, '下线', 'BUTTON', 'knowledge:article:offline', NULL, NULL, 6, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3016);

INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3017, 301, '上传附件', 'BUTTON', 'knowledge:attachment:upload', NULL, NULL, 7, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3017);
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 3018, 301, '删除附件', 'BUTTON', 'knowledge:attachment:delete', NULL, NULL, 8, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 3018);

-- ADMIN(role 1) 幂等补授新增按钮 -------------------------------------------
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, p.id
FROM sys_permission p
WHERE NOT EXISTS (
    SELECT 1 FROM sys_role_permission rp
    WHERE rp.role_id = 1 AND rp.permission_id = p.id
);
