-- =============================================================================
-- V10 模块 9 智能问答闭环：向量索引记账 + 问答会话/历史/引用/反馈（《数据库表清单》§5）
-- 约定同 V1：utf8mb4 / InnoDB；主键 id BIGINT UNSIGNED AUTO_INCREMENT；
--      实体表（qa_session）含 BaseEntity(create_by/create_time/update_by/update_time/deleted)，
--      派生/日志表（kb_chunk/kb_vector_index/qa_message/qa_reference/qa_feedback）精简；枚举一律 VARCHAR。
-- 向量库（Chroma）由 Python ai-service 持有，本处仅落 Java 编排层的元数据与问答记录。
-- RBAC 全程幂等（INSERT ... WHERE NOT EXISTS / UPDATE），不回改 V1-V9，避免 Flyway checksum 漂移。
-- =============================================================================

-- -----------------------------------------------------------------------------
-- 1. 索引来源与状态（kb_*）
-- -----------------------------------------------------------------------------

-- 向量化来源文本单元：正文方案下每篇通常 1 行（标题+摘要+正文去 HTML）；重建即整篇删旧插新，故不软删。
CREATE TABLE kb_chunk (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id  BIGINT   NOT NULL COMMENT '所属知识 kb_article.id',
  seq         INT      NOT NULL DEFAULT 0 COMMENT '切分序号(0起)',
  content     LONGTEXT COMMENT '送入向量化的文本',
  char_len    INT      NOT NULL DEFAULT 0 COMMENT '字符数',
  create_time DATETIME DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_chunk_article (article_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='向量化来源文本块';

-- 每篇向量化状态（前端「已索引/失败」展示 + 重建依据）；article_id 唯一（每篇一行，upsert）。
CREATE TABLE kb_vector_index (
  id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id      BIGINT       NOT NULL COMMENT '所属知识 kb_article.id',
  chunk_count     INT          NOT NULL DEFAULT 0 COMMENT 'Python 实际入库块数',
  dim             INT          NOT NULL DEFAULT 0 COMMENT '向量维度',
  embedding_model VARCHAR(128) DEFAULT NULL COMMENT 'embedding 模型标识(预留)',
  status          VARCHAR(16)  NOT NULL DEFAULT 'INDEXED' COMMENT 'INDEXED/REMOVED/FAILED/EMPTY',
  error_msg       VARCHAR(512) DEFAULT NULL COMMENT '失败原因(status=FAILED)',
  indexed_time    DATETIME     DEFAULT NULL COMMENT '最近成功索引时间',
  create_time     DATETIME     DEFAULT NULL,
  update_time     DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY uk_vec_article (article_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识向量化状态';

-- -----------------------------------------------------------------------------
-- 2. 问答会话与历史（qa_*）
-- -----------------------------------------------------------------------------

-- 问答会话：一个用户的一串提问归一会话；含 BaseEntity（软删，可由用户清理历史）。
CREATE TABLE qa_session (
  id            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id       BIGINT       NOT NULL COMMENT '所属用户 sys_user.id',
  title         VARCHAR(255) DEFAULT NULL COMMENT '会话标题(首问截断)',
  message_count INT          NOT NULL DEFAULT 0 COMMENT '消息数',
  create_by     BIGINT       DEFAULT NULL,
  create_time   DATETIME     DEFAULT NULL,
  update_by     BIGINT       DEFAULT NULL,
  update_time   DATETIME     DEFAULT NULL,
  deleted       TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  KEY idx_qa_session_user (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='问答会话';

-- 问答消息：一问一答一行；引用(qa_reference)与反馈(qa_feedback)挂其上。
CREATE TABLE qa_message (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  session_id  BIGINT      NOT NULL COMMENT '所属会话 qa_session.id',
  user_id     BIGINT      DEFAULT NULL COMMENT '提问用户(对外接口可空)',
  question    TEXT        NOT NULL COMMENT '问题',
  answer      LONGTEXT    COMMENT '答案',
  mode        VARCHAR(16) DEFAULT NULL COMMENT '答案来源 llm/extractive/no_hit',
  top_k       INT         DEFAULT NULL COMMENT '召回条数',
  create_time DATETIME    DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_qa_msg_session (session_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='问答消息';

-- 问答引用：某条答案命中的知识来源（标题由 Java 用 kb_article 回填，分数/片段来自 Python）。
CREATE TABLE qa_reference (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  message_id  BIGINT       NOT NULL COMMENT '所属消息 qa_message.id',
  article_id  BIGINT       DEFAULT NULL COMMENT '命中知识 kb_article.id',
  title       VARCHAR(255) DEFAULT NULL COMMENT '知识标题(回填)',
  score       DECIMAL(6,4) DEFAULT NULL COMMENT '相关度',
  snippet     VARCHAR(512) DEFAULT NULL COMMENT '命中原文片段',
  sort        INT          NOT NULL DEFAULT 0 COMMENT '展示顺序',
  create_time DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_qa_ref_msg (message_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='问答答案引用';

-- 问答反馈：某用户对某条答案的点赞/点踩；每用户每消息一条（冲突即更新）。
CREATE TABLE qa_feedback (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  message_id  BIGINT       NOT NULL COMMENT '所属消息 qa_message.id',
  user_id     BIGINT       NOT NULL COMMENT '反馈用户 sys_user.id',
  helpful     TINYINT      NOT NULL COMMENT '1赞/0踩',
  reason      VARCHAR(255) DEFAULT NULL COMMENT '点踩原因(可选)',
  create_time DATETIME     DEFAULT NULL,
  update_time DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY uk_qa_fb (message_id, user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='问答答案反馈';

-- -----------------------------------------------------------------------------
-- 3. RBAC 种子（幂等）：菜单 501 挂权限码 ai:qa；新增按钮 ai:index；补授内置角色
--    见记忆 rbac-only-admin-seeded（V2 仅授 ADMIN，新模块须给非 admin 补授）。
-- -----------------------------------------------------------------------------

-- 菜单 501（V2 已种：parent=500 智能与实时, route='qa'）补上权限码并正名。
UPDATE sys_permission SET permission_code = 'ai:qa', name = '智能问答' WHERE id = 501;

-- 重建向量索引按钮（仅管理员），挂在智能问答菜单下。
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 5011, 501, '重建向量索引', 'BUTTON', 'ai:index', NULL, NULL, 1, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 5011);

-- ADMIN(1)：500/501 V2 已授，新按钮 5011 需补（幂等带上 500/501 无害）。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, p.id FROM sys_permission p
WHERE p.id IN (500, 501, 5011)
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = 1 AND rp.permission_id = p.id);

-- 非 admin 角色 2/3/4/5：授目录 500 + 菜单 501 → 菜单可见 + ai:qa「人人可问」；
-- ai:index(5011) 仅 ADMIN，不授。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT r.role_id, p.id
FROM sys_permission p
JOIN (SELECT 2 AS role_id UNION ALL SELECT 3 UNION ALL SELECT 4 UNION ALL SELECT 5) r
WHERE p.id IN (500, 501)
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = r.role_id AND rp.permission_id = p.id);
