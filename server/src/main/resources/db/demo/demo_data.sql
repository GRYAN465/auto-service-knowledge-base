-- =============================================================================
-- 智能客服知识库系统 — 演示 / 测试数据（独立脚本，非 Flyway 迁移）
-- =============================================================================
-- 用途：为模块 4（知识管理后台）及模块 5~7（检索/互动/统计）页面提供可显示的
--       真实感数据。引用 V2 种子的真实主键：机构 id=1(总公司)、用户 id=1(admin)、
--       角色 1 ADMIN / 2 KNOWLEDGE_ADMIN / 3 AUDITOR / 4 AGENT / 5 USER。
--
-- 特性：
--   * 幂等可重跑——开头按「演示 ID 段」清理后重新插入，不动 V2 种子与运行期数据。
--   * 演示 ID 段约定：部门 1001+ / 用户 1001+ / 用户角色 1001+ / 分类 2001+ /
--     标签 3001+ / 文章 4001+ / 文章标签 41000+ / 收藏 42000+ / 分享 43000+ /
--     评论 44000+ / 点赞 45000+ / 浏览 46000+ / 搜索 47000+ / 日统计 48000+ /
--     下载 49000+ / 附件 5001+ / 版本 6001+ / 审核 7001+ / 开放应用 8001+ /
--     导入记录 9001+ / 登录日志 50000+ / 操作日志 51000+ / API日志 52000+。
--   * 所有演示账号密码均为 123456（沿用 admin 的 BCrypt 密文）。
--
-- 运行（PowerShell / cmd，注意用 utf8mb4 避免中文乱码）：
--   mysql -u kb -p123456 --default-character-set=utf8mb4 kb < server/src/main/resources/db/demo/demo_data.sql
-- 社区补充（30 条知识 + 普通用户 + 互动，需在本脚本之后执行）：
--   mysql -u kb -p123456 --default-character-set=utf8mb4 kb < server/src/main/resources/db/demo/demo_data_community.sql
-- 或在 mysql 客户端内： SOURCE .../demo/demo_data.sql;
-- =============================================================================

SET NAMES utf8mb4;
START TRANSACTION;

-- -----------------------------------------------------------------------------
-- 0. 清理旧的演示数据（仅演示 ID 段；不触碰 V2 种子与运行期手建数据）
-- -----------------------------------------------------------------------------
DELETE FROM sys_login_log       WHERE id >= 50000;
DELETE FROM sys_operation_log   WHERE id >= 51000;
DELETE FROM sys_api_log         WHERE id >= 52000;
DELETE FROM kb_download_log     WHERE id >= 49000;
DELETE FROM kb_usage_stat       WHERE id >= 48000;
DELETE FROM kb_search_log       WHERE id >= 47000;
DELETE FROM kb_view_log         WHERE id >= 46000;
DELETE FROM kb_like             WHERE id >= 45000;
DELETE FROM kb_comment          WHERE id >= 44000;
DELETE FROM kb_share            WHERE id >= 43000;
DELETE FROM kb_favorite         WHERE id >= 42000;
DELETE FROM kb_article_tag      WHERE id >= 41000;
DELETE FROM kb_audit_record     WHERE id >= 7001;
DELETE FROM kb_article_version  WHERE id >= 6001;
DELETE FROM kb_attachment       WHERE id >= 5001;
DELETE FROM kb_import_record    WHERE id >= 9001;
DELETE FROM open_app            WHERE id >= 8001;
DELETE FROM kb_article          WHERE id >= 4001;
DELETE FROM kb_tag              WHERE id >= 3001;
DELETE FROM kb_category         WHERE id >= 2001;
DELETE FROM sys_user_role       WHERE id >= 1001;
DELETE FROM sys_user            WHERE id >= 1001;
DELETE FROM sys_org             WHERE id >= 1001;

-- -----------------------------------------------------------------------------
-- 1. 机构 / 部门（挂在总公司 id=1 下）
-- -----------------------------------------------------------------------------
INSERT INTO sys_org (id, parent_id, name, code, type, sort, status, create_by, create_time, deleted) VALUES
  (1001, 1, '客服中心',   'DEMO_CS', 'DEPT', 1, 'ENABLED', 1, NOW(), 0),
  (1002, 1, '售后服务部', 'DEMO_AS', 'DEPT', 2, 'ENABLED', 1, NOW(), 0),
  (1003, 1, '知识运营部', 'DEMO_KO', 'DEPT', 3, 'ENABLED', 1, NOW(), 0),
  (1004, 1, '产品部',     'DEMO_PD', 'DEPT', 4, 'ENABLED', 1, NOW(), 0);

-- -----------------------------------------------------------------------------
-- 2. 人员（密码均为 123456）
-- -----------------------------------------------------------------------------
INSERT INTO sys_user (id, org_id, username, password, real_name, nickname, email, phone, gender, status, last_login_time, last_login_ip, remark, create_by, create_time, deleted) VALUES
  (1001, 1003, 'k_admin',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '钱知识', '小钱', 'qian@demo.com',  '13800000001', 'M', 'ENABLED',  DATE_SUB(NOW(), INTERVAL 2 HOUR), '192.168.1.21', '知识运营负责人', 1, NOW(), 0),
  (1002, 1003, 'auditor1', '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '孙审核', '老孙', 'sun@demo.com',   '13800000002', 'F', 'ENABLED',  DATE_SUB(NOW(), INTERVAL 1 DAY),  '192.168.1.22', '内容审核',     1, NOW(), 0),
  (1003, 1001, 'agent1',   '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '周坐席', '小周', 'zhou@demo.com',  '13800000003', 'M', 'ENABLED',  DATE_SUB(NOW(), INTERVAL 30 MINUTE), '192.168.1.23', '一线坐席',  1, NOW(), 0),
  (1004, 1001, 'agent2',   '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '吴坐席', '小吴', 'wu@demo.com',    '13800000004', 'F', 'ENABLED',  DATE_SUB(NOW(), INTERVAL 3 HOUR), '192.168.1.24', '一线坐席',  1, NOW(), 0),
  (1005, 1002, 'agent3',   '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '郑坐席', '小郑', 'zheng@demo.com', '13800000005', 'M', 'DISABLED', DATE_SUB(NOW(), INTERVAL 10 DAY), '192.168.1.25', '售后坐席(停用示例)', 1, NOW(), 0),
  (1006, 1004, 'user1',    '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '王用户', '小王', 'wang@demo.com',  '13800000006', 'F', 'ENABLED',  DATE_SUB(NOW(), INTERVAL 5 HOUR), '192.168.1.26', '普通浏览用户', 1, NOW(), 0);

-- 用户-角色：k_admin→知识管理员, auditor1→审核员, agent1/2/3→坐席, user1→普通用户
INSERT INTO sys_user_role (id, user_id, role_id) VALUES
  (1001, 1001, 2),
  (1002, 1002, 3),
  (1003, 1003, 4),
  (1004, 1004, 4),
  (1005, 1005, 4),
  (1006, 1006, 5);

-- -----------------------------------------------------------------------------
-- 3. 知识分类（树）
-- -----------------------------------------------------------------------------
INSERT INTO kb_category (id, parent_id, name, code, sort, status, create_by, create_time, deleted) VALUES
  (2001, 0,    '话术库',     'CAT_SCRIPT',  1, 'ENABLED', 1001, NOW(), 0),
  (2011, 2001, '售前咨询话术','CAT_PRESALE', 1, 'ENABLED', 1001, NOW(), 0),
  (2012, 2001, '售后处理话术','CAT_AFTSALE', 2, 'ENABLED', 1001, NOW(), 0),
  (2013, 2001, '投诉应对话术','CAT_COMPLAIN',3, 'ENABLED', 1001, NOW(), 0),
  (2002, 0,    '产品知识',   'CAT_PRODUCT', 2, 'ENABLED', 1001, NOW(), 0),
  (2021, 2002, '套餐资费',   'CAT_FEE',     1, 'ENABLED', 1001, NOW(), 0),
  (2022, 2002, '功能说明',   'CAT_FEATURE', 2, 'ENABLED', 1001, NOW(), 0),
  (2003, 0,    '业务流程',   'CAT_FLOW',    3, 'ENABLED', 1001, NOW(), 0),
  (2031, 2003, '开户与激活', 'CAT_OPEN',    1, 'ENABLED', 1001, NOW(), 0),
  (2032, 2003, '退订与销户', 'CAT_CANCEL',  2, 'ENABLED', 1001, NOW(), 0),
  (2004, 0,    '培训资料',   'CAT_TRAIN',   4, 'ENABLED', 1001, NOW(), 0),
  (2005, 0,    '办公制度',   'CAT_OFFICE',  5, 'ENABLED', 1001, NOW(), 0);

-- -----------------------------------------------------------------------------
-- 4. 知识标签
-- -----------------------------------------------------------------------------
INSERT INTO kb_tag (id, name, color, sort, create_by, create_time, deleted) VALUES
  (3001, '高频',     '#2563EB', 1, 1001, NOW(), 0),
  (3002, '新人必读', '#16A34A', 2, 1001, NOW(), 0),
  (3003, '易错点',   '#DC2626', 3, 1001, NOW(), 0),
  (3004, '退订',     '#F59E0B', 4, 1001, NOW(), 0),
  (3005, '投诉',     '#DB2777', 5, 1001, NOW(), 0),
  (3006, '资费',     '#0891B2', 6, 1001, NOW(), 0),
  (3007, '激活',     '#7C3AED', 7, 1001, NOW(), 0),
  (3008, 'VIP',      '#CA8A04', 8, 1001, NOW(), 0),
  (3009, '售前',     '#0EA5E9', 9, 1001, NOW(), 0),
  (3010, '售后',     '#10B981', 10, 1001, NOW(), 0),
  (3011, '5G',       '#6366F1', 11, 1001, NOW(), 0),
  (3012, '宽带',     '#8B5CF6', 12, 1001, NOW(), 0),
  (3013, '融合套餐', '#EC4899', 13, 1001, NOW(), 0),
  (3014, '携号转网', '#14B8A6', 14, 1001, NOW(), 0),
  (3015, '实名认证', '#F97316', 15, 1001, NOW(), 0),
  (3016, '发票',     '#64748B', 16, 1001, NOW(), 0),
  (3017, '故障报修', '#EF4444', 17, 1001, NOW(), 0),
  (3018, '积分',     '#A855F7', 18, 1001, NOW(), 0),
  (3019, '漫游',     '#06B6D4', 19, 1001, NOW(), 0),
  (3020, '物联网',   '#84CC16', 20, 1001, NOW(), 0),
  (3021, '政企',     '#475569', 21, 1001, NOW(), 0),
  (3022, '校园',     '#F43F5E', 22, 1001, NOW(), 0);

-- -----------------------------------------------------------------------------
-- 5. 知识文章（覆盖全部状态：ONLINE / PENDING_AUDIT / DRAFT / OFFLINE / REJECTED）
--    冗余计数（view/like/favorite/comment）与下方日志/统计大致对应。
-- -----------------------------------------------------------------------------
INSERT INTO kb_article (id, title, category_id, knowledge_type, summary, content, status, source, author_id, current_version, online_time, offline_time, offline_reason, view_count, like_count, dislike_count, favorite_count, comment_count, create_by, create_time, update_by, update_time, deleted) VALUES
  (4001, '宽带报装标准话术', 2011, 'SCRIPT', '客户来电咨询宽带安装时的标准应答流程与话术模板。',
    '<h3>开场</h3><p>您好，很高兴为您服务。请问您要咨询宽带报装吗？</p><h3>核实信息</h3><p>请提供您的安装地址与联系电话，我帮您查询是否已覆盖光纤。</p><h3>结束语</h3><p>已为您登记，48 小时内安装师傅会与您联系，感谢您的来电。</p>',
    'ONLINE', 'MANUAL', 1001, 2, DATE_SUB(NOW(), INTERVAL 12 DAY), NULL, NULL, 386, 42, 1, 8, 3, 1001, DATE_SUB(NOW(), INTERVAL 14 DAY), 1001, DATE_SUB(NOW(), INTERVAL 12 DAY), 0),
  (4002, '套餐资费一览（2026版）', 2021, 'PRODUCT', '2026 年最新在售套餐与资费对照表，含融合套餐与单宽带。',
    '<h3>融合套餐</h3><ul><li>畅享 99 元：100M 宽带 + 30GB 流量</li><li>畅享 159 元：500M 宽带 + 60GB 流量</li></ul><h3>单宽带</h3><p>200M 包年 1200 元，赠装机。</p>',
    'ONLINE', 'MANUAL', 1001, 3, DATE_SUB(NOW(), INTERVAL 10 DAY), NULL, NULL, 642, 73, 2, 15, 2, 1001, DATE_SUB(NOW(), INTERVAL 15 DAY), 1001, DATE_SUB(NOW(), INTERVAL 4 DAY), 0),
  (4003, '客户要求退订的应对流程', 2032, 'SCRIPT', '处理客户退订诉求的安抚话术、挽留策略与办理步骤。',
    '<h3>第一步 安抚</h3><p>理解您的想法，请问是哪方面让您不太满意呢？</p><h3>第二步 挽留</h3><p>我们可以为您申请一份专属优惠……</p><h3>第三步 办理</h3><p>如仍坚持退订，需本人核身后于次月生效。</p>',
    'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 8 DAY), NULL, NULL, 295, 31, 4, 6, 1, 1001, DATE_SUB(NOW(), INTERVAL 9 DAY), 1001, DATE_SUB(NOW(), INTERVAL 8 DAY), 0),
  (4004, '投诉升级处理 SOP', 2013, 'SCRIPT', '一线无法解决的投诉如何升级、记录与回访的标准流程。',
    '<h3>分级</h3><p>普通投诉 24h 内回复；重大投诉 2h 内升级主管。</p><h3>记录</h3><p>在工单系统填写诉求、承诺时限与处理人。</p>',
    'ONLINE', 'MANUAL', 1002, 2, DATE_SUB(NOW(), INTERVAL 7 DAY), NULL, NULL, 218, 27, 0, 5, 2, 1002, DATE_SUB(NOW(), INTERVAL 8 DAY), 1002, DATE_SUB(NOW(), INTERVAL 7 DAY), 0),
  (4005, '新员工服务礼仪培训', 2004, 'TRAIN', '面向新入职坐席的服务礼仪、用语规范与禁忌培训资料。',
    '<h3>三声服务</h3><p>来有迎声、问有答声、走有送声。</p><h3>禁语</h3><p>不允许使用「不知道」「这不归我管」等推诿用语。</p>',
    'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 6 DAY), NULL, NULL, 174, 19, 0, 4, 1, 1001, DATE_SUB(NOW(), INTERVAL 6 DAY), 1001, DATE_SUB(NOW(), INTERVAL 6 DAY), 0),
  (4006, 'VIP 客户专属服务权益', 2002, 'PRODUCT', 'VIP 客户的优先接入、专属客服与增值权益说明。',
    '<h3>权益</h3><ul><li>专属热线优先接入</li><li>故障 4 小时上门</li><li>每年 2 次免费提速</li></ul>',
    'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 5 DAY), NULL, NULL, 132, 16, 1, 7, 0, 1001, DATE_SUB(NOW(), INTERVAL 5 DAY), 1001, DATE_SUB(NOW(), INTERVAL 5 DAY), 0),
  (4007, '号码激活操作指引', 2031, 'SCRIPT', '新号码现场/远程激活的操作步骤与常见失败原因。',
    '<h3>激活步骤</h3><p>核身 → 录入资料 → 系统激活 → 测试拨打。</p><h3>常见失败</h3><p>身份证过期、占用号段、未实名。</p>',
    'ONLINE', 'MANUAL', 1003, 1, DATE_SUB(NOW(), INTERVAL 4 DAY), NULL, NULL, 88, 9, 0, 2, 0, 1003, DATE_SUB(NOW(), INTERVAL 4 DAY), 1003, DATE_SUB(NOW(), INTERVAL 4 DAY), 0),
  (4008, '2026 年办公考勤制度', 2005, 'OFFICE', '坐席排班、迟到早退、请假与调休的考勤管理制度。',
    '<h3>排班</h3><p>实行三班轮换，提前一周公布班表。</p><h3>请假</h3><p>事假需提前 1 天在系统提交并经主管审批。</p>',
    'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 3 DAY), NULL, NULL, 61, 5, 0, 1, 0, 1001, DATE_SUB(NOW(), INTERVAL 3 DAY), 1001, DATE_SUB(NOW(), INTERVAL 3 DAY), 0),
  (4009, '新功能「智能质检」介绍', 2022, 'PRODUCT', '待审核：新上线的通话智能质检功能说明（提交审核中）。',
    '<h3>功能简介</h3><p>对坐席通话自动转写并按规则打分，辅助质检。</p>',
    'PENDING_AUDIT', 'MANUAL', 1001, 1, NULL, NULL, NULL, 0, 0, 0, 0, 0, 1001, DATE_SUB(NOW(), INTERVAL 1 DAY), 1001, DATE_SUB(NOW(), INTERVAL 1 DAY), 0),
  (4010, '夏季营销话术（草稿）', 2012, 'SCRIPT', '草稿：暑期促销活动的外呼话术，尚未完成编写。',
    '<p>草稿待补充……</p>',
    'DRAFT', 'MANUAL', 1001, 1, NULL, NULL, NULL, 0, 0, 0, 0, 0, 1001, DATE_SUB(NOW(), INTERVAL 2 DAY), 1001, DATE_SUB(NOW(), INTERVAL 2 DAY), 0),
  (4011, '旧资费说明（已下线）', 2021, 'PRODUCT', '已下线：2024 版资费，已被 2026 版替代。',
    '<p>本资费已停止销售，请参阅《套餐资费一览（2026版）》。</p>',
    'OFFLINE', 'MANUAL', 1001, 2, DATE_SUB(NOW(), INTERVAL 60 DAY), DATE_SUB(NOW(), INTERVAL 11 DAY), '资费调整，已被 2026 版替代', 410, 22, 6, 3, 0, 1001, DATE_SUB(NOW(), INTERVAL 65 DAY), 1001, DATE_SUB(NOW(), INTERVAL 11 DAY), 0),
  (4012, '历史话术导入稿（格式不规范）', 2004, 'TRAIN', '驳回：批量导入的历史话术，因格式与分类缺失被驳回。',
    '<p>（导入原文，排版混乱）</p>',
    'REJECTED', 'IMPORT', 1001, 1, NULL, NULL, NULL, 0, 0, 0, 0, 0, 1001, DATE_SUB(NOW(), INTERVAL 5 DAY), 1002, DATE_SUB(NOW(), INTERVAL 4 DAY), 0);

-- -----------------------------------------------------------------------------
-- 6. 文章-标签关系
-- -----------------------------------------------------------------------------
INSERT INTO kb_article_tag (id, article_id, tag_id) VALUES
  (41001, 4001, 3001), (41002, 4001, 3002),
  (41003, 4002, 3001), (41004, 4002, 3006),
  (41005, 4003, 3004), (41006, 4003, 3003),
  (41007, 4004, 3005), (41008, 4004, 3001),
  (41009, 4005, 3002),
  (41010, 4006, 3008),
  (41011, 4007, 3007),
  (41012, 4011, 3006);

-- -----------------------------------------------------------------------------
-- 7. 附件
-- -----------------------------------------------------------------------------
INSERT INTO kb_attachment (id, article_id, file_name, file_path, file_type, file_size, storage_type, download_count, create_by, create_time, deleted) VALUES
  (5001, 4002, '2026套餐资费表.pdf',   '/storage/demo/2026_fee.pdf',       'PDF',  524288,  'LOCAL', 37, 1001, DATE_SUB(NOW(), INTERVAL 10 DAY), 0),
  (5002, 4005, '服务礼仪培训.pptx',     '/storage/demo/etiquette.pptx',     'PPT',  2097152, 'LOCAL', 12, 1001, DATE_SUB(NOW(), INTERVAL 6 DAY), 0),
  (5003, 4008, '2026考勤制度.docx',     '/storage/demo/attendance.docx',    'WORD', 65536,   'LOCAL', 5,  1001, DATE_SUB(NOW(), INTERVAL 3 DAY), 0);

-- -----------------------------------------------------------------------------
-- 8. 文章版本快照（与 current_version 对应）
-- -----------------------------------------------------------------------------
INSERT INTO kb_article_version (id, article_id, version_no, title, content, editor_id, change_log, create_time) VALUES
  (6001, 4001, 1, '宽带报装标准话术', '<p>初版话术。</p>',                 1001, '初稿',           DATE_SUB(NOW(), INTERVAL 14 DAY)),
  (6002, 4001, 2, '宽带报装标准话术', '<p>补充结束语与时限承诺。</p>',     1001, '补充结束语',     DATE_SUB(NOW(), INTERVAL 12 DAY)),
  (6003, 4002, 1, '套餐资费一览',     '<p>2026 初版资费。</p>',            1001, '初稿',           DATE_SUB(NOW(), INTERVAL 15 DAY)),
  (6004, 4002, 2, '套餐资费一览',     '<p>新增单宽带包年。</p>',           1001, '新增单宽带',     DATE_SUB(NOW(), INTERVAL 8 DAY)),
  (6005, 4002, 3, '套餐资费一览（2026版）', '<p>调整融合套餐价格。</p>',    1001, '调价',           DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (6006, 4004, 1, '投诉升级处理 SOP', '<p>初版分级与记录。</p>',           1002, '初稿',           DATE_SUB(NOW(), INTERVAL 8 DAY)),
  (6007, 4004, 2, '投诉升级处理 SOP', '<p>补充回访要求。</p>',             1002, '补充回访',       DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (6008, 4011, 1, '旧资费说明',       '<p>2024 版资费。</p>',              1001, '初稿',           DATE_SUB(NOW(), INTERVAL 65 DAY));

-- -----------------------------------------------------------------------------
-- 9. 审核记录（PASS / REJECT / 待审 submit-only）
-- -----------------------------------------------------------------------------
INSERT INTO kb_audit_record (id, article_id, submit_by, auditor_id, audit_status, audit_opinion, submit_time, audit_time) VALUES
  (7001, 4001, 1001, 1002, 'PASS',   '话术规范，准予上线',           DATE_SUB(NOW(), INTERVAL 13 DAY), DATE_SUB(NOW(), INTERVAL 12 DAY)),
  (7002, 4002, 1001, 1002, 'PASS',   '资费核对无误',                 DATE_SUB(NOW(), INTERVAL 11 DAY), DATE_SUB(NOW(), INTERVAL 10 DAY)),
  (7003, 4004, 1002, 1002, 'PASS',   'SOP 清晰',                     DATE_SUB(NOW(), INTERVAL 8 DAY),  DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (7004, 4012, 1001, 1002, 'REJECT', '格式混乱、未填分类，退回重整', DATE_SUB(NOW(), INTERVAL 5 DAY),  DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (7005, 4009, 1001, NULL, NULL,     NULL,                           DATE_SUB(NOW(), INTERVAL 1 DAY),  NULL);

-- -----------------------------------------------------------------------------
-- 10. 收藏
-- -----------------------------------------------------------------------------
INSERT INTO kb_favorite (id, article_id, user_id, folder, create_time) VALUES
  (42001, 4001, 1003, '常用话术', DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (42002, 4002, 1003, '常用话术', DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (42003, 4004, 1003, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (42004, 4002, 1004, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (42005, 4006, 1004, 'VIP 资料', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (42006, 4005, 1006, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 1 DAY));

-- -----------------------------------------------------------------------------
-- 11. 分享
-- -----------------------------------------------------------------------------
INSERT INTO kb_share (id, article_id, from_user_id, to_user_id, share_type, message, read_status, create_time) VALUES
  (43001, 4001, 1003, 1004, 'USER', '这个开场话术不错，参考下', 1, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (43002, 4004, 1002, 1003, 'USER', '投诉升级按这个 SOP 走',   0, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (43003, 4002, 1004, 1005, 'USER', '最新资费表',               0, DATE_SUB(NOW(), INTERVAL 1 DAY));

-- -----------------------------------------------------------------------------
-- 12. 评论（含二级回复：parent_id 指向上一条评论）
-- -----------------------------------------------------------------------------
INSERT INTO kb_comment (id, article_id, user_id, parent_id, content, status, like_count, create_time, deleted) VALUES
  (44001, 4001, 1003, 0,     '这个话术很实用，照着说客户体验好很多。', 'NORMAL', 5, DATE_SUB(NOW(), INTERVAL 5 DAY), 0),
  (44002, 4001, 1001, 44001, '感谢反馈～后续会再补充几个场景。',        'NORMAL', 1, DATE_SUB(NOW(), INTERVAL 5 DAY), 0),
  (44003, 4001, 1004, 0,     '结束语能不能再口语化一点？',              'NORMAL', 0, DATE_SUB(NOW(), INTERVAL 4 DAY), 0),
  (44004, 4002, 1005, 0,     '资费表很清晰，建议加个对比图。',          'NORMAL', 2, DATE_SUB(NOW(), INTERVAL 3 DAY), 0),
  (44005, 4004, 1004, 0,     '重大投诉 2 小时升级，这点很关键。',       'NORMAL', 3, DATE_SUB(NOW(), INTERVAL 2 DAY), 0),
  (44006, 4004, 1006, 0,     '（已隐藏的违规评论示例）',                'HIDDEN', 0, DATE_SUB(NOW(), INTERVAL 2 DAY), 0);

-- -----------------------------------------------------------------------------
-- 13. 点赞 / 点踩（target_type ARTICLE/COMMENT；type 1 赞 / -1 踩）
-- -----------------------------------------------------------------------------
INSERT INTO kb_like (id, target_type, target_id, user_id, type, create_time) VALUES
  (45001, 'ARTICLE', 4001, 1003, 1,  DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (45002, 'ARTICLE', 4001, 1004, 1,  DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (45003, 'ARTICLE', 4001, 1006, 1,  DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45004, 'ARTICLE', 4002, 1003, 1,  DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45005, 'ARTICLE', 4002, 1005, 1,  DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (45006, 'ARTICLE', 4003, 1004, -1, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (45007, 'ARTICLE', 4011, 1006, -1, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (45008, 'COMMENT', 44001, 1004, 1, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45009, 'COMMENT', 44001, 1006, 1, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (45010, 'COMMENT', 44005, 1003, 1, DATE_SUB(NOW(), INTERVAL 2 DAY));

-- -----------------------------------------------------------------------------
-- 14. 浏览记录（近 14 天，支撑统计趋势）
-- -----------------------------------------------------------------------------
INSERT INTO kb_view_log (id, article_id, user_id, client_ip, stay_seconds, create_time) VALUES
  (46001, 4001, 1003, '192.168.1.23', 95,  DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46002, 4001, 1004, '192.168.1.24', 42,  DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46003, 4001, 1006, '192.168.1.26', 130, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46004, 4002, 1003, '192.168.1.23', 210, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46005, 4002, 1004, '192.168.1.24', 180, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46006, 4002, 1005, '192.168.1.25', 60,  DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46007, 4002, 1006, '192.168.1.26', 75,  DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46008, 4003, 1004, '192.168.1.24', 88,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46009, 4003, 1003, '192.168.1.23', 50,  DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (46010, 4004, 1004, '192.168.1.24', 99,  DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46011, 4004, 1006, '192.168.1.26', 45,  DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (46012, 4005, 1006, '192.168.1.26', 150, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46013, 4006, 1004, '192.168.1.24', 70,  DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46014, 4007, 1003, '192.168.1.23', 33,  DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (46015, 4002, 1003, '192.168.1.23', 120, DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (46016, 4001, 1005, '192.168.1.25', 64,  DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (46017, 4004, 1003, '192.168.1.23', 81,  DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (46018, 4002, 1006, '192.168.1.26', 95,  DATE_SUB(NOW(), INTERVAL 8 DAY)),
  (46019, 4001, 1004, '192.168.1.24', 40,  DATE_SUB(NOW(), INTERVAL 9 DAY)),
  (46020, 4003, 1005, '192.168.1.25', 55,  DATE_SUB(NOW(), INTERVAL 10 DAY));

-- -----------------------------------------------------------------------------
-- 15. 搜索日志
-- -----------------------------------------------------------------------------
INSERT INTO kb_search_log (id, user_id, keyword, result_count, client_ip, create_time) VALUES
  (47001, 1003, '退订',   2, '192.168.1.23', DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (47002, 1004, '资费',   3, '192.168.1.24', DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (47003, 1003, '投诉',   1, '192.168.1.23', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (47004, 1005, '激活',   1, '192.168.1.25', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (47005, 1006, 'VIP',    1, '192.168.1.26', DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (47006, 1004, '宽带',   1, '192.168.1.24', DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (47007, 1003, '套餐',   2, '192.168.1.23', DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (47008, 1006, '不存在的词', 0, '192.168.1.26', DATE_SUB(NOW(), INTERVAL 4 DAY));

-- -----------------------------------------------------------------------------
-- 16. 按天使用统计（近 5 天 × 热门文章；唯一键 article_id+stat_date）
-- -----------------------------------------------------------------------------
INSERT INTO kb_usage_stat (id, article_id, stat_date, view_count, like_count, favorite_count, share_count, comment_count, download_count, create_time, update_time) VALUES
  (48001, 4001, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 38, 4, 1, 1, 1, 0, NOW(), NOW()),
  (48002, 4001, DATE_SUB(CURDATE(), INTERVAL 2 DAY), 45, 6, 2, 0, 0, 0, NOW(), NOW()),
  (48003, 4001, DATE_SUB(CURDATE(), INTERVAL 3 DAY), 52, 5, 1, 1, 1, 0, NOW(), NOW()),
  (48004, 4002, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 61, 8, 3, 1, 1, 5, NOW(), NOW()),
  (48005, 4002, DATE_SUB(CURDATE(), INTERVAL 2 DAY), 73, 9, 2, 0, 0, 7, NOW(), NOW()),
  (48006, 4002, DATE_SUB(CURDATE(), INTERVAL 3 DAY), 58, 7, 2, 1, 0, 4, NOW(), NOW()),
  (48007, 4004, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 29, 3, 1, 1, 1, 0, NOW(), NOW()),
  (48008, 4004, DATE_SUB(CURDATE(), INTERVAL 2 DAY), 34, 4, 1, 0, 1, 0, NOW(), NOW()),
  (48009, 4003, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 22, 2, 0, 0, 0, 0, NOW(), NOW()),
  (48010, 4005, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 18, 2, 1, 0, 0, 3, NOW(), NOW());

-- -----------------------------------------------------------------------------
-- 17. 下载日志
-- -----------------------------------------------------------------------------
INSERT INTO kb_download_log (id, article_id, attachment_id, user_id, download_type, client_ip, create_time) VALUES
  (49001, 4002, 5001, 1003, 'SINGLE', '192.168.1.23', DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (49002, 4002, 5001, 1004, 'SINGLE', '192.168.1.24', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (49003, 4005, 5002, 1006, 'SINGLE', '192.168.1.26', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (49004, 4008, 5003, 1003, 'SINGLE', '192.168.1.23', DATE_SUB(NOW(), INTERVAL 3 DAY));

-- -----------------------------------------------------------------------------
-- 18. 开放应用（模块 8）
-- -----------------------------------------------------------------------------
INSERT INTO open_app (id, app_name, app_key, app_secret, status, rate_limit, scope, remark, create_by, create_time, deleted) VALUES
  (8001, '客服移动端',   'demo_app_key_mobile_0001', 'demo_secret_mobile_aaaaaaaaaaaaaaaa', 'ENABLED',  120, 'search,detail,qa', '内部移动端接入', 1, DATE_SUB(NOW(), INTERVAL 20 DAY), 0),
  (8002, '第三方合作门户', 'demo_app_key_portal_0002', 'demo_secret_portal_bbbbbbbbbbbbbbbb', 'DISABLED', 60,  'search,detail',    '合作方(已停用示例)', 1, DATE_SUB(NOW(), INTERVAL 18 DAY), 0);

-- -----------------------------------------------------------------------------
-- 19. 导入记录（模块 4 批量导入）
-- -----------------------------------------------------------------------------
INSERT INTO kb_import_record (id, file_name, total_count, success_count, fail_count, status, fail_detail, operator_id, create_time) VALUES
  (9001, '历史话术导入_批次1.xlsx', 50, 48, 2, 'DONE', '第 12 行分类缺失；第 37 行标题超长', 1001, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (9002, '产品手册导入.xlsx',       30, 0,  0, 'RUNNING', NULL,                              1001, DATE_SUB(NOW(), INTERVAL 1 HOUR));

-- -----------------------------------------------------------------------------
-- 20. 登录日志（系统日志页展示）
-- -----------------------------------------------------------------------------
INSERT INTO sys_login_log (id, user_id, username, login_ip, login_location, client, status, msg, login_time) VALUES
  (50001, 1003, 'agent1',   '192.168.1.23', '内网', 'QtClient', 'SUCCESS', '登录成功', DATE_SUB(NOW(), INTERVAL 30 MINUTE)),
  (50002, 1004, 'agent2',   '192.168.1.24', '内网', 'QtClient', 'SUCCESS', '登录成功', DATE_SUB(NOW(), INTERVAL 3 HOUR)),
  (50003, 1001, 'k_admin',  '192.168.1.21', '内网', 'QtClient', 'SUCCESS', '登录成功', DATE_SUB(NOW(), INTERVAL 2 HOUR)),
  (50004, NULL, 'agent3',   '192.168.1.25', '内网', 'QtClient', 'FAIL',    '账号已停用', DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (50005, NULL, 'unknown',  '203.0.113.10', '外网', 'QtClient', 'FAIL',    '用户名或密码错误', DATE_SUB(NOW(), INTERVAL 6 HOUR));

-- -----------------------------------------------------------------------------
-- 21. 操作日志（系统日志页展示）
-- -----------------------------------------------------------------------------
INSERT INTO sys_operation_log (id, user_id, username, module, operation, method, request_uri, request_param, status, error_msg, cost_time, operation_time) VALUES
  (51001, 1001, 'k_admin', '知识管理', '新增知识', 'POST',   '/api/knowledge/article', '{"title":"VIP 客户专属服务权益"}', 'SUCCESS', NULL, 86,  DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (51002, 1002, 'auditor1','审核中心', '审核驳回', 'POST',   '/api/knowledge/article/4012/audit', '{"status":"REJECT"}', 'SUCCESS', NULL, 54, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (51003, 1001, 'k_admin', '知识管理', '上线知识', 'POST',   '/api/knowledge/article/4002/online', '{}', 'SUCCESS', NULL, 70, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (51004, 1001, 'k_admin', '知识分类', '新增分类', 'POST',   '/api/knowledge/category', '{"name":"功能说明"}', 'SUCCESS', NULL, 41, DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (51005, 1, 'admin', '系统管理', '新增用户', 'POST', '/api/system/user', '{"username":"agent3"}', 'SUCCESS', NULL, 63, DATE_SUB(NOW(), INTERVAL 10 DAY));

-- -----------------------------------------------------------------------------
-- 22. 开放 API 调用日志（模块 8 / 统计）
-- -----------------------------------------------------------------------------
INSERT INTO sys_api_log (id, app_id, app_key, api_path, http_method, request_param, response_code, client_ip, cost_time, create_time) VALUES
  (52001, 8001, 'demo_app_key_mobile_0001', '/openapi/search', 'GET', '{"kw":"资费"}', 200, '10.0.0.5', 35, DATE_SUB(NOW(), INTERVAL 1 HOUR)),
  (52002, 8001, 'demo_app_key_mobile_0001', '/openapi/detail', 'GET', '{"id":4002}',   200, '10.0.0.5', 28, DATE_SUB(NOW(), INTERVAL 2 HOUR)),
  (52003, 8001, 'demo_app_key_mobile_0001', '/openapi/search', 'GET', '{"kw":"退订"}', 200, '10.0.0.6', 31, DATE_SUB(NOW(), INTERVAL 5 HOUR)),
  (52004, 8002, 'demo_app_key_portal_0002', '/openapi/search', 'GET', '{"kw":"宽带"}', 403, '198.51.100.7', 9, DATE_SUB(NOW(), INTERVAL 1 DAY));

COMMIT;

-- =============================================================================
-- 演示数据装载完成。回滚：重跑本脚本前部的 DELETE 段即可（或整段重跑覆盖）。
-- 演示账号：k_admin / auditor1 / agent1 / agent2 / agent3(停用) / user1，密码均为 123456。
-- =============================================================================
