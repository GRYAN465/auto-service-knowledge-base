-- =============================================================================
-- V14 社区投稿完善：编辑草稿、管理附件（与知识管理同一套接口能力）
-- =============================================================================

INSERT INTO sys_role_permission (role_id, permission_id)
SELECT r.role_id, p.id
FROM sys_permission p
JOIN (SELECT 4 AS role_id UNION ALL SELECT 5 AS role_id) r
WHERE p.permission_code IN (
        'knowledge:article:update',
        'knowledge:attachment:upload',
        'knowledge:attachment:delete'
      )
  AND NOT EXISTS (
        SELECT 1 FROM sys_role_permission rp
        WHERE rp.role_id = r.role_id AND rp.permission_id = p.id
      );
