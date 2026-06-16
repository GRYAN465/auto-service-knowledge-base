-- =============================================================================
-- V2 RBAC 种子数据：根机构 + 超级管理员 + 内置角色 + 菜单/权限码
-- 菜单 route 与权限码与 MeResponse.stubAdmin / 《API契约.md》对齐；ADMIN 授予全部权限。
-- 注意：admin 初始密码为 123456（BCrypt），上线前请改密。
-- =============================================================================

-- 1. 根机构 ----------------------------------------------------------------
INSERT INTO sys_org (id, parent_id, name, code, type, sort, status, create_by, create_time, deleted)
VALUES (1, 0, '总公司', 'ROOT', 'COMPANY', 1, 'ENABLED', 1, NOW(), 0);

-- 2. 超级管理员账号（密码 123456 的 BCrypt 密文）-----------------------------
INSERT INTO sys_user (id, org_id, username, password, real_name, nickname, gender, status, remark, create_by, create_time, deleted)
VALUES (1, 1, 'admin', '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi',
        '管理员', '管理员', 'U', 'ENABLED', '初始超级管理员', 1, NOW(), 0);

-- 3. 内置角色 --------------------------------------------------------------
INSERT INTO sys_role (id, name, code, data_scope, sort, status, remark, create_by, create_time, deleted) VALUES
  (1, '系统管理员', 'ADMIN',           'ALL',  1, 'ENABLED', '拥有全部权限',     1, NOW(), 0),
  (2, '知识管理员', 'KNOWLEDGE_ADMIN', 'DEPT', 2, 'ENABLED', '知识录入与管理',   1, NOW(), 0),
  (3, '审核员',     'AUDITOR',         'ALL',  3, 'ENABLED', '知识审核上下线',   1, NOW(), 0),
  (4, '坐席',       'AGENT',           'SELF', 4, 'ENABLED', '检索/收藏/问答',   1, NOW(), 0),
  (5, '普通用户',   'USER',            'SELF', 5, 'ENABLED', '基础知识浏览',     1, NOW(), 0);

-- 4. admin -> ADMIN ---------------------------------------------------------
INSERT INTO sys_user_role (user_id, role_id) VALUES (1, 1);

-- 5. 菜单 / 权限码（树）-----------------------------------------------------
-- 顶级目录 DIR（100~600）
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time) VALUES
  (100, 0, '工作台',         'DIR', NULL, NULL, 'dashboard', 1, 'ENABLED', NOW()),
  (200, 0, '知识应用',       'DIR', NULL, NULL, 'app',       2, 'ENABLED', NOW()),
  (300, 0, '知识管理',       'DIR', NULL, NULL, 'book',      3, 'ENABLED', NOW()),
  (400, 0, '统计与开放',     'DIR', NULL, NULL, 'chart',     4, 'ENABLED', NOW()),
  (500, 0, '智能与实时(二期)','DIR', NULL, NULL, 'robot',     5, 'ENABLED', NOW()),
  (600, 0, '系统管理',       'DIR', NULL, NULL, 'setting',   6, 'ENABLED', NOW());

-- 菜单 MENU
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time) VALUES
  (101, 100, '首页仪表盘',           'MENU', NULL,                       'dashboard',          'home',    1, 'ENABLED', NOW()),
  (201, 200, '知识检索',             'MENU', 'knowledge:search',         'knowledge.search',   'search',  1, 'ENABLED', NOW()),
  (202, 200, '我的收藏',             'MENU', 'interaction:favorite',     'favorite',           'star',    2, 'ENABLED', NOW()),
  (203, 200, '我的分享',             'MENU', 'interaction:share',        'share',              'share',   3, 'ENABLED', NOW()),
  (301, 300, '知识管理',             'MENU', 'knowledge:article:list',   'knowledge.manage',   'book',    1, 'ENABLED', NOW()),
  (302, 300, '审核中心',             'MENU', 'knowledge:article:audit',  'audit',              'audit',   2, 'ENABLED', NOW()),
  (303, 300, '分类标签管理',         'MENU', 'knowledge:category:list',  'category',           'tags',    3, 'ENABLED', NOW()),
  (401, 400, '数据统计',             'MENU', 'statistics:view',          'statistics',         'chart',   1, 'ENABLED', NOW()),
  (402, 400, 'API 开放管理',         'MENU', 'openapi:app:list',         'openapi',            'api',     2, 'ENABLED', NOW()),
  (501, 500, '知识问答（二期）',     'MENU', NULL,                       'qa',                 'robot',   1, 'ENABLED', NOW()),
  (502, 500, '坐席辅助台（二期）',   'MENU', NULL,                       'agent',              'headset', 2, 'ENABLED', NOW()),
  (503, 500, 'CC/ASR 接入配置（二期）','MENU', NULL,                     'cc.config',          'phone',   3, 'ENABLED', NOW()),
  (601, 600, '机构人员',             'MENU', 'system:user:list',         'system.user',        'team',    1, 'ENABLED', NOW()),
  (602, 600, '角色管理',             'MENU', 'system:role:list',         'system.role',        'role',    2, 'ENABLED', NOW()),
  (603, 600, '菜单按钮权限',         'MENU', 'system:permission:list',   'system.permission',  'lock',    3, 'ENABLED', NOW()),
  (604, 600, '系统日志',             'MENU', 'system:log:operation',     'system.log',         'log',     4, 'ENABLED', NOW());

-- 按钮 BUTTON
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time) VALUES
  (2011, 201, '发表评论', 'BUTTON', 'interaction:comment',      NULL, NULL, 1, 'ENABLED', NOW()),
  (3011, 301, '新增知识', 'BUTTON', 'knowledge:article:create', NULL, NULL, 1, 'ENABLED', NOW()),
  (3012, 301, '编辑知识', 'BUTTON', 'knowledge:article:update', NULL, NULL, 2, 'ENABLED', NOW()),
  (3013, 301, '上下线',   'BUTTON', 'knowledge:article:online', NULL, NULL, 3, 'ENABLED', NOW()),
  (3031, 303, '标签管理', 'BUTTON', 'knowledge:tag:list',       NULL, NULL, 1, 'ENABLED', NOW()),
  (6011, 601, '新增用户', 'BUTTON', 'system:user:create',       NULL, NULL, 1, 'ENABLED', NOW()),
  (6012, 601, '编辑用户', 'BUTTON', 'system:user:update',       NULL, NULL, 2, 'ENABLED', NOW()),
  (6013, 601, '删除用户', 'BUTTON', 'system:user:delete',       NULL, NULL, 3, 'ENABLED', NOW()),
  (6041, 604, '登录日志', 'BUTTON', 'system:log:login',         NULL, NULL, 1, 'ENABLED', NOW());

-- 6. ADMIN 角色授予全部权限 -------------------------------------------------
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, id FROM sys_permission;
