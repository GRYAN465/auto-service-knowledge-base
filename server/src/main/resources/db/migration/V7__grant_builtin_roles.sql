-- =============================================================================
-- V7 内置角色授权（修复种子缺口）
-- V2 仅给 ADMIN(role 1) 授全权，KNOWLEDGE_ADMIN(2)/AUDITOR(3)/AGENT(4)/USER(5)
-- 此前无任何权限码 —— 坐席等非 admin 账号访问受保护端点一律 403。
-- 本迁移按职责为这 4 个内置角色幂等补授权限码（按 permission_code 关联，DIR 目录按 id），
-- 全部 INSERT ... SELECT ... WHERE NOT EXISTS，可重复执行。
-- 不回改 V1-V6，避免已执行环境发生 Flyway checksum 变化。ADMIN 授权仍由 V2~V6 维护。
-- 权限码以 V2~V6 已种入者为准；新模块（7 统计 / 8 开放 API）的角色授权留待对应迁移补。
-- =============================================================================

-- 2 知识管理员 KNOWLEDGE_ADMIN：知识录入与管理（分类/标签/知识 CRUD + 附件 + 检索 + 互动）；
--   不含审核/上下线（归审核员）与系统管理。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 2, p.id FROM sys_permission p
WHERE (
        p.id IN (200, 300)  -- 目录：知识应用 / 知识管理
        OR p.permission_code IN (
            'knowledge:search',
            'interaction:favorite', 'interaction:share', 'interaction:comment', 'interaction:like',
            'knowledge:article:list', 'knowledge:article:create', 'knowledge:article:update',
            'knowledge:article:delete', 'knowledge:article:submit',
            'knowledge:attachment:upload', 'knowledge:attachment:delete',
            'knowledge:category:list', 'knowledge:category:create', 'knowledge:category:update', 'knowledge:category:delete',
            'knowledge:tag:list', 'knowledge:tag:create', 'knowledge:tag:update', 'knowledge:tag:delete'
        )
      )
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = 2 AND rp.permission_id = p.id);

-- 3 审核员 AUDITOR：知识审核 + 上下线 + 检索 + 互动（可见知识管理列表以便审核）。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 3, p.id FROM sys_permission p
WHERE (
        p.id IN (200, 300)  -- 目录：知识应用 / 知识管理
        OR p.permission_code IN (
            'knowledge:search',
            'interaction:favorite', 'interaction:share', 'interaction:comment', 'interaction:like',
            'knowledge:article:list', 'knowledge:article:audit',
            'knowledge:article:online', 'knowledge:article:offline'
        )
      )
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = 3 AND rp.permission_id = p.id);

-- 4 坐席 AGENT、5 普通用户 USER：门户消费侧——检索 + 收藏/点赞踩/评论/分享（站内通知）。
--   二者权限相同（均为知识门户阅读+互动），如后续需区分再细化。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT r.role_id, p.id
FROM sys_permission p
JOIN (SELECT 4 AS role_id UNION ALL SELECT 5 AS role_id) r
WHERE (
        p.id = 200  -- 目录：知识应用
        OR p.permission_code IN (
            'knowledge:search',
            'interaction:favorite', 'interaction:share', 'interaction:comment', 'interaction:like'
        )
      )
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = r.role_id AND rp.permission_id = p.id);
