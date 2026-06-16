-- =============================================================================
-- V1 一期初始化：系统管理 + 知识 + 互动 + 统计 + 开放能力 全部表
-- 依据《数据库表清单.md》（一期 26 张表）。二期表（qa_/cc_/asr_/rt_）以 V3+ 增量添加。
-- 约定：utf8mb4 / InnoDB；主键 id BIGINT UNSIGNED AUTO_INCREMENT；
--      实体表含 BaseEntity(create_by/create_time/update_by/update_time/deleted)，
--      关系表 / 日志表不含；枚举一律 VARCHAR。
-- =============================================================================

-- -----------------------------------------------------------------------------
-- 1. 系统管理 sys_*
-- -----------------------------------------------------------------------------

CREATE TABLE sys_org (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  parent_id   BIGINT       NOT NULL DEFAULT 0 COMMENT '上级机构，根为0',
  name        VARCHAR(100) NOT NULL COMMENT '机构名称',
  code        VARCHAR(64)  NOT NULL COMMENT '机构编码',
  type        VARCHAR(20)  NOT NULL DEFAULT 'DEPT' COMMENT 'COMPANY/DEPT',
  sort        INT          NOT NULL DEFAULT 0 COMMENT '排序',
  status      VARCHAR(16)  NOT NULL DEFAULT 'ENABLED' COMMENT 'ENABLED/DISABLED',
  create_by   BIGINT       DEFAULT NULL,
  create_time DATETIME     DEFAULT NULL,
  update_by   BIGINT       DEFAULT NULL,
  update_time DATETIME     DEFAULT NULL,
  deleted     TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  UNIQUE KEY uk_org_code (code),
  KEY idx_org_parent (parent_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='机构/部门表(树)';

CREATE TABLE sys_user (
  id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  org_id          BIGINT       DEFAULT NULL COMMENT '所属机构',
  username        VARCHAR(64)  NOT NULL COMMENT '登录名',
  password        VARCHAR(100) NOT NULL COMMENT 'BCrypt 密文',
  real_name       VARCHAR(64)  DEFAULT NULL COMMENT '姓名',
  nickname        VARCHAR(64)  DEFAULT NULL COMMENT '昵称',
  avatar          VARCHAR(255) DEFAULT NULL COMMENT '头像url',
  email           VARCHAR(128) DEFAULT NULL COMMENT '邮箱',
  phone           VARCHAR(32)  DEFAULT NULL COMMENT '手机号',
  gender          VARCHAR(8)   DEFAULT 'U' COMMENT 'M/F/U',
  status          VARCHAR(16)  NOT NULL DEFAULT 'ENABLED' COMMENT 'ENABLED/DISABLED',
  expire_time     DATETIME     DEFAULT NULL COMMENT '账号有效期',
  last_login_time DATETIME     DEFAULT NULL COMMENT '最近登录时间',
  last_login_ip   VARCHAR(64)  DEFAULT NULL COMMENT '最近登录IP',
  remark          VARCHAR(255) DEFAULT NULL,
  create_by       BIGINT       DEFAULT NULL,
  create_time     DATETIME     DEFAULT NULL,
  update_by       BIGINT       DEFAULT NULL,
  update_time     DATETIME     DEFAULT NULL,
  deleted         TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  UNIQUE KEY uk_user_username (username),
  KEY idx_user_org (org_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='人员账号表';

CREATE TABLE sys_role (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  name        VARCHAR(64)  NOT NULL COMMENT '角色名',
  code        VARCHAR(64)  NOT NULL COMMENT '角色编码 如 ADMIN/AUDITOR',
  data_scope  VARCHAR(16)  NOT NULL DEFAULT 'SELF' COMMENT '数据权限 ALL/DEPT/SELF(预留)',
  sort        INT          NOT NULL DEFAULT 0,
  status      VARCHAR(16)  NOT NULL DEFAULT 'ENABLED',
  remark      VARCHAR(255) DEFAULT NULL,
  create_by   BIGINT       DEFAULT NULL,
  create_time DATETIME     DEFAULT NULL,
  update_by   BIGINT       DEFAULT NULL,
  update_time DATETIME     DEFAULT NULL,
  deleted     TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  UNIQUE KEY uk_role_code (code)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='角色表';

CREATE TABLE sys_permission (
  id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  parent_id       BIGINT       NOT NULL DEFAULT 0 COMMENT '上级，根为0',
  name            VARCHAR(64)  NOT NULL COMMENT '名称',
  type            VARCHAR(16)  NOT NULL COMMENT 'DIR/MENU/BUTTON',
  permission_code VARCHAR(128) DEFAULT NULL COMMENT '权限码 如 knowledge:article:create',
  route           VARCHAR(128) DEFAULT NULL COMMENT '客户端页面name(MENU用)',
  icon            VARCHAR(64)  DEFAULT NULL,
  sort            INT          NOT NULL DEFAULT 0,
  status          VARCHAR(16)  NOT NULL DEFAULT 'ENABLED',
  create_by       BIGINT       DEFAULT NULL,
  create_time     DATETIME     DEFAULT NULL,
  update_by       BIGINT       DEFAULT NULL,
  update_time     DATETIME     DEFAULT NULL,
  deleted         TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  KEY idx_perm_parent (parent_id),
  KEY idx_perm_code (permission_code)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='菜单/按钮权限表(树)';

CREATE TABLE sys_user_role (
  id      BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id BIGINT NOT NULL,
  role_id BIGINT NOT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY uk_user_role (user_id, role_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='用户-角色关系';

CREATE TABLE sys_role_permission (
  id            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  role_id       BIGINT NOT NULL,
  permission_id BIGINT NOT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY uk_role_perm (role_id, permission_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='角色-权限关系';

CREATE TABLE sys_login_log (
  id             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id        BIGINT       DEFAULT NULL,
  username       VARCHAR(64)  DEFAULT NULL,
  login_ip       VARCHAR(64)  DEFAULT NULL,
  login_location VARCHAR(128) DEFAULT NULL,
  client         VARCHAR(64)  DEFAULT NULL COMMENT '客户端类型',
  status         VARCHAR(16)  DEFAULT NULL COMMENT 'SUCCESS/FAIL',
  msg            VARCHAR(255) DEFAULT NULL,
  login_time     DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_login_user (user_id),
  KEY idx_login_time (login_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='登录日志';

CREATE TABLE sys_operation_log (
  id             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id        BIGINT       DEFAULT NULL,
  username       VARCHAR(64)  DEFAULT NULL,
  module         VARCHAR(64)  DEFAULT NULL,
  operation      VARCHAR(128) DEFAULT NULL,
  method         VARCHAR(255) DEFAULT NULL,
  request_uri    VARCHAR(255) DEFAULT NULL,
  request_param  TEXT         DEFAULT NULL,
  status         VARCHAR(16)  DEFAULT NULL,
  error_msg      TEXT         DEFAULT NULL,
  cost_time      BIGINT       DEFAULT NULL COMMENT '耗时ms',
  operation_time DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_op_user (user_id),
  KEY idx_op_time (operation_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='操作日志';

CREATE TABLE sys_api_log (
  id            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  app_id        BIGINT       DEFAULT NULL,
  app_key       VARCHAR(64)  DEFAULT NULL,
  api_path      VARCHAR(255) DEFAULT NULL,
  http_method   VARCHAR(16)  DEFAULT NULL,
  request_param TEXT         DEFAULT NULL,
  response_code INT          DEFAULT NULL,
  client_ip     VARCHAR(64)  DEFAULT NULL,
  cost_time     BIGINT       DEFAULT NULL COMMENT '耗时ms',
  create_time   DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_apilog_app (app_id),
  KEY idx_apilog_time (create_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='开放API调用日志';

-- -----------------------------------------------------------------------------
-- 2. 知识管理 / 应用 kb_*
-- -----------------------------------------------------------------------------

CREATE TABLE kb_category (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  parent_id   BIGINT       NOT NULL DEFAULT 0 COMMENT '上级分类，根为0',
  name        VARCHAR(100) NOT NULL,
  code        VARCHAR(64)  DEFAULT NULL,
  sort        INT          NOT NULL DEFAULT 0,
  status      VARCHAR(16)  NOT NULL DEFAULT 'ENABLED',
  create_by   BIGINT       DEFAULT NULL,
  create_time DATETIME     DEFAULT NULL,
  update_by   BIGINT       DEFAULT NULL,
  update_time DATETIME     DEFAULT NULL,
  deleted     TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  KEY idx_cat_parent (parent_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识分类表(树)';

CREATE TABLE kb_tag (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  name        VARCHAR(64)  NOT NULL,
  color       VARCHAR(16)  DEFAULT NULL,
  sort        INT          NOT NULL DEFAULT 0,
  create_by   BIGINT       DEFAULT NULL,
  create_time DATETIME     DEFAULT NULL,
  update_by   BIGINT       DEFAULT NULL,
  update_time DATETIME     DEFAULT NULL,
  deleted     TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  UNIQUE KEY uk_tag_name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识标签表';

CREATE TABLE kb_article (
  id              BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  title           VARCHAR(200) NOT NULL,
  category_id     BIGINT       DEFAULT NULL,
  knowledge_type  VARCHAR(32)  DEFAULT NULL COMMENT 'SCRIPT/TRAIN/PRODUCT/OFFICE',
  summary         VARCHAR(500) DEFAULT NULL,
  content         LONGTEXT     DEFAULT NULL COMMENT '富文本正文HTML',
  status          VARCHAR(20)  NOT NULL DEFAULT 'DRAFT' COMMENT 'DRAFT/PENDING_AUDIT/ONLINE/OFFLINE/REJECTED',
  source          VARCHAR(16)  NOT NULL DEFAULT 'MANUAL' COMMENT 'MANUAL/IMPORT',
  author_id       BIGINT       DEFAULT NULL,
  current_version INT          NOT NULL DEFAULT 1,
  online_time     DATETIME     DEFAULT NULL,
  offline_time    DATETIME     DEFAULT NULL,
  offline_reason  VARCHAR(255) DEFAULT NULL,
  view_count      BIGINT       NOT NULL DEFAULT 0,
  like_count      BIGINT       NOT NULL DEFAULT 0,
  dislike_count   BIGINT       NOT NULL DEFAULT 0,
  favorite_count  BIGINT       NOT NULL DEFAULT 0,
  comment_count   BIGINT       NOT NULL DEFAULT 0,
  create_by       BIGINT       DEFAULT NULL,
  create_time     DATETIME     DEFAULT NULL,
  update_by       BIGINT       DEFAULT NULL,
  update_time     DATETIME     DEFAULT NULL,
  deleted         TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  KEY idx_art_category (category_id),
  KEY idx_art_status (status),
  KEY idx_art_author (author_id),
  FULLTEXT KEY ft_art_content (title, summary, content) WITH PARSER ngram
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识主表';

CREATE TABLE kb_article_tag (
  id         BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id BIGINT NOT NULL,
  tag_id     BIGINT NOT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY uk_art_tag (article_id, tag_id),
  KEY idx_arttag_tag (tag_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识-标签关系';

CREATE TABLE kb_attachment (
  id             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id     BIGINT       NOT NULL,
  file_name      VARCHAR(255) NOT NULL COMMENT '原始文件名',
  file_path      VARCHAR(512) NOT NULL COMMENT '存储路径',
  file_type      VARCHAR(32)  DEFAULT NULL COMMENT 'PDF/WORD/PPT/AUDIO/VIDEO/OTHER',
  file_size      BIGINT       DEFAULT NULL COMMENT '字节',
  storage_type   VARCHAR(16)  NOT NULL DEFAULT 'LOCAL' COMMENT 'LOCAL(后续MINIO)',
  download_count BIGINT       NOT NULL DEFAULT 0,
  create_by      BIGINT       DEFAULT NULL,
  create_time    DATETIME     DEFAULT NULL,
  update_by      BIGINT       DEFAULT NULL,
  update_time    DATETIME     DEFAULT NULL,
  deleted        TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  KEY idx_att_article (article_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='附件表';

CREATE TABLE kb_article_version (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id  BIGINT       NOT NULL,
  version_no  INT          NOT NULL COMMENT '版本号',
  title       VARCHAR(200) DEFAULT NULL,
  content     LONGTEXT     DEFAULT NULL COMMENT '内容快照',
  editor_id   BIGINT       DEFAULT NULL,
  change_log  VARCHAR(500) DEFAULT NULL,
  create_time DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_ver_article (article_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识版本表';

CREATE TABLE kb_audit_record (
  id            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id    BIGINT       NOT NULL,
  submit_by     BIGINT       DEFAULT NULL,
  auditor_id    BIGINT       DEFAULT NULL,
  audit_status  VARCHAR(16)  DEFAULT NULL COMMENT 'PASS/REJECT',
  audit_opinion VARCHAR(500) DEFAULT NULL,
  submit_time   DATETIME     DEFAULT NULL,
  audit_time    DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_audit_article (article_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识审核记录表';

CREATE TABLE kb_import_record (
  id            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  file_name     VARCHAR(255) DEFAULT NULL,
  total_count   INT          NOT NULL DEFAULT 0,
  success_count INT          NOT NULL DEFAULT 0,
  fail_count    INT          NOT NULL DEFAULT 0,
  status        VARCHAR(16)  DEFAULT NULL COMMENT 'RUNNING/DONE/FAIL',
  fail_detail   TEXT         DEFAULT NULL,
  operator_id   BIGINT       DEFAULT NULL,
  create_time   DATETIME     DEFAULT NULL,
  PRIMARY KEY (id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识导入记录表';

CREATE TABLE kb_download_log (
  id            BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id    BIGINT      DEFAULT NULL,
  attachment_id BIGINT      DEFAULT NULL,
  user_id       BIGINT      DEFAULT NULL,
  download_type VARCHAR(16) DEFAULT NULL COMMENT 'SINGLE/BATCH/ZIP',
  client_ip     VARCHAR(64) DEFAULT NULL,
  create_time   DATETIME    DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_dl_article (article_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识下载日志表';

CREATE TABLE kb_favorite (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id  BIGINT      NOT NULL,
  user_id     BIGINT      NOT NULL,
  folder      VARCHAR(64) DEFAULT NULL COMMENT '收藏夹',
  create_time DATETIME    DEFAULT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY uk_fav_art_user (article_id, user_id),
  KEY idx_fav_user (user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识收藏表';

CREATE TABLE kb_share (
  id           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id   BIGINT       NOT NULL,
  from_user_id BIGINT       DEFAULT NULL,
  to_user_id   BIGINT       DEFAULT NULL,
  share_type   VARCHAR(16)  DEFAULT NULL,
  message      VARCHAR(255) DEFAULT NULL,
  read_status  TINYINT      NOT NULL DEFAULT 0 COMMENT '0未读/1已读',
  create_time  DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_share_to (to_user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识分享表';

CREATE TABLE kb_comment (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id  BIGINT      NOT NULL,
  user_id     BIGINT      DEFAULT NULL,
  parent_id   BIGINT      NOT NULL DEFAULT 0 COMMENT '回复的父评论，0为顶层',
  content     TEXT        NOT NULL,
  status      VARCHAR(16) NOT NULL DEFAULT 'NORMAL' COMMENT 'NORMAL/HIDDEN',
  like_count  BIGINT      NOT NULL DEFAULT 0,
  create_time DATETIME    DEFAULT NULL,
  deleted     TINYINT     NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  KEY idx_cmt_article (article_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识评论表';

CREATE TABLE kb_like (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  target_type VARCHAR(16) NOT NULL COMMENT 'ARTICLE/COMMENT',
  target_id   BIGINT      NOT NULL,
  user_id     BIGINT      NOT NULL,
  type        TINYINT     NOT NULL COMMENT '1赞/-1踩',
  create_time DATETIME    DEFAULT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY uk_like (target_type, target_id, user_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='点赞/点踩表';

CREATE TABLE kb_view_log (
  id           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id   BIGINT      NOT NULL,
  user_id      BIGINT      DEFAULT NULL,
  client_ip    VARCHAR(64) DEFAULT NULL,
  stay_seconds INT         DEFAULT NULL COMMENT '停留秒数',
  create_time  DATETIME    DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_view_article (article_id),
  KEY idx_view_time (create_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识浏览记录表';

CREATE TABLE kb_search_log (
  id           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  user_id      BIGINT       DEFAULT NULL,
  keyword      VARCHAR(255) DEFAULT NULL,
  result_count INT          DEFAULT NULL,
  client_ip    VARCHAR(64)  DEFAULT NULL,
  create_time  DATETIME     DEFAULT NULL,
  PRIMARY KEY (id),
  KEY idx_search_time (create_time)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识搜索日志表';

CREATE TABLE kb_usage_stat (
  id             BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  article_id     BIGINT NOT NULL,
  stat_date      DATE   NOT NULL COMMENT '统计日',
  view_count     BIGINT NOT NULL DEFAULT 0,
  like_count     BIGINT NOT NULL DEFAULT 0,
  favorite_count BIGINT NOT NULL DEFAULT 0,
  share_count    BIGINT NOT NULL DEFAULT 0,
  comment_count  BIGINT NOT NULL DEFAULT 0,
  download_count BIGINT NOT NULL DEFAULT 0,
  create_time    DATETIME DEFAULT NULL,
  update_time    DATETIME DEFAULT NULL,
  PRIMARY KEY (id),
  UNIQUE KEY uk_stat_art_date (article_id, stat_date)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='知识使用统计表(按天)';

-- -----------------------------------------------------------------------------
-- 3. 开放能力 open_*
-- -----------------------------------------------------------------------------

CREATE TABLE open_app (
  id          BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  app_name    VARCHAR(100) NOT NULL,
  app_key     VARCHAR(64)  NOT NULL,
  app_secret  VARCHAR(128) NOT NULL,
  status      VARCHAR(16)  NOT NULL DEFAULT 'ENABLED',
  rate_limit  INT          NOT NULL DEFAULT 60 COMMENT '限流 次/分钟',
  scope       VARCHAR(255) DEFAULT NULL COMMENT '授权范围 如 search,detail,qa',
  remark      VARCHAR(255) DEFAULT NULL,
  create_by   BIGINT       DEFAULT NULL,
  create_time DATETIME     DEFAULT NULL,
  update_by   BIGINT       DEFAULT NULL,
  update_time DATETIME     DEFAULT NULL,
  deleted     TINYINT      NOT NULL DEFAULT 0,
  PRIMARY KEY (id),
  UNIQUE KEY uk_app_key (app_key)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='开放应用表';
