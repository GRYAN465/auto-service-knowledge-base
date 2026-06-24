-- =============================================================================
-- V13 知识社区投稿：坐席/普通用户可新建知识并提交审核
-- 复用模块 4 已有接口 POST /knowledge/article + POST /{id}/submit；
-- 另授分类/标签只读列表，供投稿表单选择。
-- =============================================================================

INSERT INTO sys_role_permission (role_id, permission_id)
SELECT r.role_id, p.id
FROM sys_permission p
JOIN (SELECT 4 AS role_id UNION ALL SELECT 5 AS role_id) r
WHERE p.permission_code IN (
        'knowledge:article:create',
        'knowledge:article:submit',
        'knowledge:category:list',
        'knowledge:tag:list'
      )
  AND NOT EXISTS (
        SELECT 1 FROM sys_role_permission rp
        WHERE rp.role_id = r.role_id AND rp.permission_id = p.id
      );
