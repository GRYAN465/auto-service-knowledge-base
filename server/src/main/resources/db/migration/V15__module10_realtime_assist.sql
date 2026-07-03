-- =============================================================================
-- V15 模块 10 客服实时辅助（CC）：Mock ASR 转写 + WebSocket 实时推送 + 定时 RAG 推荐。
--   会话复用 qa_session（title 前缀「实时辅助-」区分），转写独立建表 rt_transcript
--   （qa_message 是一问一答结构、无 role/content，不适配转写语义，故不复用）。
--   活跃会话 rt_active_session：sessionId 唯一、坐席维度索引、last_check_time 供定时推荐循环。
-- RBAC：复用 V2 已预留但未授权的菜单 502《坐席辅助台》（route=agent / icon=headset / parent=500），
--   补权限码 realtime:assist、正名为「坐席辅助」；新增按钮 5021；幂等授 ADMIN(1)+AGENT(4)。
-- 约定同 V10：utf8mb4 / InnoDB；幂等 INSERT ... WHERE NOT EXISTS / UPDATE，不回改 V1-V14。
-- =============================================================================

-- -----------------------------------------------------------------------------
-- 1. 活跃实时辅助会话（rt_active_session）
-- -----------------------------------------------------------------------------
CREATE TABLE rt_active_session (
  id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  session_id      BIGINT       NOT NULL COMMENT '关联 qa_session.id',
  agent_user_id   BIGINT       NOT NULL COMMENT '坐席用户 sys_user.id',
  caller_number   VARCHAR(64)  DEFAULT NULL COMMENT '主叫号码',
  cc_call_id      VARCHAR(128) DEFAULT NULL COMMENT 'CC 通话 ID（Mock 阶段可空）',
  status          VARCHAR(20)  NOT NULL DEFAULT 'ACTIVE' COMMENT 'ACTIVE / ENDED',
  last_check_time DATETIME     DEFAULT NULL COMMENT '上次推荐检查时间(排除已检索转写)',
  create_time     DATETIME     NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  UNIQUE KEY uk_session (session_id),
  KEY idx_agent (agent_user_id),
  KEY idx_status (status)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='活跃实时辅助会话';

-- -----------------------------------------------------------------------------
-- 2. 实时转写片段（rt_transcript）：替代复用 qa_message（详见头部说明）
-- -----------------------------------------------------------------------------
CREATE TABLE rt_transcript (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  session_id  BIGINT      NOT NULL COMMENT '关联 qa_session.id（经 rt_active_session.session_id）',
  speaker     VARCHAR(16) NOT NULL COMMENT 'customer / agent',
  content     TEXT        NOT NULL COMMENT '转写文本',
  seq_no      INT         NOT NULL DEFAULT 0 COMMENT '片段序号(0起，按此排序回放)',
  create_time DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (id),
  KEY idx_rt_transcript_session (session_id, seq_no)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='实时辅助转写片段';

-- -----------------------------------------------------------------------------
-- 3. RBAC 种子（幂等）
-- -----------------------------------------------------------------------------

-- 3.1 正名并补权限码到预留菜单 502（V2 已种：parent=500, route='agent', icon='headset'）。
UPDATE sys_permission
SET permission_code = 'realtime:assist', name = '坐席辅助'
WHERE id = 502 AND (permission_code IS NULL OR permission_code = '' OR name LIKE '%（二期）%');

-- 3.2 新增按钮 5021：realtime:assist（挂在 502 下，与菜单同权限码，方法级鉴权用）。
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 5021, 502, '实时辅助操作', 'BUTTON', 'realtime:assist', NULL, NULL, 1, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 5021);

-- 3.3 ADMIN(1)：补授 502/5021（500 已由 V2 授；幂等带上 500 无害）。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, p.id FROM sys_permission p
WHERE p.id IN (500, 502, 5021)
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = 1 AND rp.permission_id = p.id);

-- 3.4 AGENT(4)：授目录 500 + 菜单 502 + 按钮 5021（坐席默认可用实时辅助）。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 4, p.id FROM sys_permission p
WHERE p.id IN (500, 502, 5021)
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = 4 AND rp.permission_id = p.id);