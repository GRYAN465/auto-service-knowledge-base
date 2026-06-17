-- =============================================================================
-- V11 模块 9 LLM 运行时配置：管理员在 Qt 客户端填写/测试/保存 OpenAI 兼容接入参数，
--      Java 落库（权威源）并实时下发 Python（无需重启）。单行配置，恒定操作 id=1。
-- 约定同 V10：utf8mb4 / InnoDB；主键 id BIGINT UNSIGNED AUTO_INCREMENT；
--      本表非软删（单行配置，无删除语义）。api_key 本地明文存（同 yml 内 jwt secret 级别），
--      对外只回掩码、绝不回传明文。
-- RBAC 全程幂等（INSERT ... WHERE NOT EXISTS），不回改 V1-V10，避免 Flyway checksum 漂移。
-- =============================================================================

-- -----------------------------------------------------------------------------
-- 1. LLM 接入配置（单行，id=1）
-- -----------------------------------------------------------------------------

CREATE TABLE ai_llm_config (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  base_url    VARCHAR(255)  DEFAULT NULL COMMENT 'OpenAI 兼容端点(通常以 /v1 结尾)',
  api_key     VARCHAR(512)  DEFAULT NULL COMMENT 'API 密钥(本地明文存，对外掩码)',
  model       VARCHAR(128)  DEFAULT NULL COMMENT '模型名',
  temperature DECIMAL(3,2)  NOT NULL DEFAULT 0.20 COMMENT '采样温度',
  max_tokens  INT           NOT NULL DEFAULT 1024 COMMENT '最大生成 token 数',
  enabled     TINYINT       NOT NULL DEFAULT 0 COMMENT '是否启用 1启用/0停用',
  update_by   BIGINT        DEFAULT NULL COMMENT '最后修改人 sys_user.id',
  create_time DATETIME      DEFAULT NULL,
  update_time DATETIME      DEFAULT NULL,
  PRIMARY KEY (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='LLM 运行时接入配置(单行)';

-- 种一行空配置（GET/PUT 恒定操作 id=1）。
INSERT INTO ai_llm_config (id, base_url, api_key, model, temperature, max_tokens, enabled, create_time, update_time)
SELECT 1, NULL, NULL, NULL, 0.20, 1024, 0, NOW(), NOW()
WHERE NOT EXISTS (SELECT 1 FROM ai_llm_config WHERE id = 1);

-- -----------------------------------------------------------------------------
-- 2. RBAC 种子（幂等）：新增菜单 504 ai:config，仅授 ADMIN → 天然管理员可见。
--    见记忆 rbac-only-admin-seeded：菜单驱动由 /auth/me 决定，不授非 admin 即对其隐藏。
-- -----------------------------------------------------------------------------

-- AI 配置菜单（parent=500 智能与实时；route 'ai.config'；500/501/502/503 已占，本菜单用 504）。
INSERT INTO sys_permission (id, parent_id, name, type, permission_code, route, icon, sort, status, create_time)
SELECT 504, 500, 'AI 配置', 'MENU', 'ai:config', 'ai.config', 'setting', 4, 'ENABLED', NOW()
WHERE NOT EXISTS (SELECT 1 FROM sys_permission WHERE id = 504);

-- 仅 ADMIN(1) 授 504（不授非 admin → 该菜单仅管理员可见）。
INSERT INTO sys_role_permission (role_id, permission_id)
SELECT 1, p.id FROM sys_permission p
WHERE p.id IN (504)
  AND NOT EXISTS (SELECT 1 FROM sys_role_permission rp WHERE rp.role_id = 1 AND rp.permission_id = p.id);
