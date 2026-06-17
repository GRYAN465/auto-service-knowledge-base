-- =============================================================================
-- V8 模块 7 数据统计 角色授权（补 V7 遗留的新模块授权）
-- 权限码 statistics:view 与菜单 401「数据统计」、目录 400「统计与开放」均已在 V2 种入，
-- 且 ADMIN(role 1) 已随 V2 末尾「ADMIN 授全部权限」覆盖——本迁移无需新增权限码。
-- V7 明确将「新模块（7 统计 / 8 开放 API）的非 admin 角色授权」留待对应迁移补，此处补模块 7：
--   仅授后台分析角色——知识管理员(2) 与 审核员(3)；坐席(4)/普通用户(5) 为门户消费侧，不开放统计。
-- 同时授予目录 400，使「数据统计」菜单在导航树中有父节点可挂（菜单树按 parent 嵌套）。
-- 全部 INSERT ... SELECT ... WHERE NOT EXISTS，可重复执行；不回改 V1-V7。
-- =============================================================================

-- 2 知识管理员 KNOWLEDGE_ADMIN、3 审核员 AUDITOR：开放数据统计（只读分析）。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT r.role_id, p.id
FROM sys_permission p
JOIN (SELECT 2 AS role_id UNION ALL SELECT 3 AS role_id) r
WHERE (
        p.id = 400  -- 目录：统计与开放（仅作菜单父节点，子项各自按权限码显隐）
        OR p.permission_code = 'statistics:view'
      )
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = r.role_id AND rp.permission_id = p.id);
