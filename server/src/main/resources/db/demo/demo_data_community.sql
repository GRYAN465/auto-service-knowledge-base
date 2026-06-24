-- =============================================================================
-- 智能客服知识库 — 社区演示数据补充（独立脚本，与 demo_data.sql 并存）
-- =============================================================================
-- 用途：在 demo_data.sql 基础上追加普通用户、30 条已上线知识及互动数据，
--       用于知识社区、个人中心、热门统计等页面的真实感展示。
--
-- ID 段约定（不与 demo_data.sql 冲突）：
--   用户 1101–1110 / 用户角色 1101–1110
--   文章 4101–4130（30 条 ONLINE）
--   文章标签 41101+ / 收藏 42101+ / 评论 44101+ / 点赞 45101+
--   浏览 46101+ / 搜索 47101+ / 日统计 48101+
--
-- 前置：需先执行 demo_data.sql（或至少存在 V2 种子 + 分类/标签 demo 段）。
-- 密码：新增账号均为 123456（与 admin 相同 BCrypt）。
--
-- 运行：
--   mysql -u kb -p123456 --default-character-set=utf8mb4 kb < server/src/main/resources/db/demo/demo_data_community.sql
-- 可选后续：demo_data_batch2.sql（+5 用户 +50 知识）
-- =============================================================================

SET NAMES utf8mb4;
START TRANSACTION;

-- -----------------------------------------------------------------------------
-- 0. 清理本脚本 ID 段（幂等重跑）
-- -----------------------------------------------------------------------------
DELETE FROM kb_usage_stat    WHERE id >= 48101;
DELETE FROM kb_search_log    WHERE id >= 47101;
DELETE FROM kb_view_log      WHERE id >= 46101;
DELETE FROM kb_like          WHERE id >= 45101;
DELETE FROM kb_comment       WHERE id >= 44101;
DELETE FROM kb_favorite      WHERE id >= 42101;
DELETE FROM kb_article_tag   WHERE id >= 41101;
DELETE FROM kb_article       WHERE id >= 4101 AND id <= 4130;
DELETE FROM sys_user_role    WHERE id >= 1101 AND id <= 1110;
DELETE FROM sys_user         WHERE id >= 1101 AND id <= 1110;

-- -----------------------------------------------------------------------------
-- 1. 普通用户（角色 5 = USER，密码 123456）
-- -----------------------------------------------------------------------------
INSERT INTO sys_user (id, org_id, username, password, real_name, nickname, email, phone, gender, status, last_login_time, last_login_ip, remark, create_by, create_time, deleted) VALUES
  (1101, 1001, 'user2',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '李阅读', '小李', 'li@demo.com',    '13900000001', 'M', 'ENABLED', DATE_SUB(NOW(), INTERVAL 2 HOUR),  '192.168.2.11', '社区活跃用户', 1, NOW(), 0),
  (1102, 1001, 'user3',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '陈浏览', '小陈', 'chen@demo.com',  '13900000002', 'F', 'ENABLED', DATE_SUB(NOW(), INTERVAL 5 HOUR),  '192.168.2.12', '常搜资费类',   1, NOW(), 0),
  (1103, 1002, 'user4',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '刘收藏', '小刘', 'liu@demo.com',   '13900000003', 'F', 'ENABLED', DATE_SUB(NOW(), INTERVAL 1 DAY),   '192.168.2.13', '收藏达人',     1, NOW(), 0),
  (1104, 1001, 'user5',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '赵评论', '小赵', 'zhao@demo.com',  '13900000004', 'M', 'ENABLED', DATE_SUB(NOW(), INTERVAL 3 HOUR),  '192.168.2.14', '爱留言互动',   1, NOW(), 0),
  (1105, 1002, 'user6',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '黄新手', '小黄', 'huang@demo.com', '13900000005', 'U', 'ENABLED', DATE_SUB(NOW(), INTERVAL 6 DAY),   '192.168.2.15', '新入职浏览',   1, NOW(), 0),
  (1106, 1001, 'user7',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '林查询', '小林', 'lin@demo.com',   '13900000006', 'M', 'ENABLED', DATE_SUB(NOW(), INTERVAL 12 HOUR), '192.168.2.16', '检索高频',     1, NOW(), 0),
  (1107, 1003, 'user8',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '何分享', '小何', 'he@demo.com',    '13900000007', 'F', 'ENABLED', DATE_SUB(NOW(), INTERVAL 4 DAY),   '192.168.2.17', '偶尔分享',     1, NOW(), 0),
  (1108, 1001, 'user9',  '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '徐学习', '小徐', 'xu@demo.com',    '13900000008', 'M', 'ENABLED', DATE_SUB(NOW(), INTERVAL 8 HOUR),  '192.168.2.18', '培训资料读者', 1, NOW(), 0),
  (1109, 1002, 'user10', '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '马访客', '小马', 'ma@demo.com',    '13900000009', 'F', 'ENABLED', DATE_SUB(NOW(), INTERVAL 2 DAY),   '192.168.2.19', '轻度浏览',     1, NOW(), 0),
  (1110, 1001, 'user11', '$2a$10$N.zmdr9k7uOCQb376NoUnuTJ8iAt6Z5EHsM8lE9lBOsl7iKTVKIUi', '朱点赞', '小朱', 'zhu@demo.com',   '13900000010', 'M', 'ENABLED', DATE_SUB(NOW(), INTERVAL 1 HOUR),  '192.168.2.20', '点赞较多',     1, NOW(), 0);

INSERT INTO sys_user_role (id, user_id, role_id) VALUES
  (1101, 1101, 5), (1102, 1102, 5), (1103, 1103, 5), (1104, 1104, 5), (1105, 1105, 5),
  (1106, 1106, 5), (1107, 1107, 5), (1108, 1108, 5), (1109, 1109, 5), (1110, 1110, 5);

-- -----------------------------------------------------------------------------
-- 2. 30 条已上线知识（4101–4130；计数与下方互动记录大致对应）
-- -----------------------------------------------------------------------------
INSERT INTO kb_article (id, title, category_id, knowledge_type, summary, content, status, source, author_id, current_version, online_time, view_count, like_count, dislike_count, favorite_count, comment_count, create_by, create_time, update_by, update_time, deleted) VALUES
  (4101, '5G套餐升级咨询标准话术', 2011, 'SCRIPT', '客户咨询从4G升5G套餐时的需求确认、资费对比与办理指引。',
    '<h3>开场</h3><p>您好，5G套餐网速更快、延迟更低，我帮您看看哪款适合您。</p><h3>对比</h3><p>请说明当前套餐与月消费，我为您推荐同档或升档方案。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 20 DAY), 523, 67, 2, 18, 6, 1001, DATE_SUB(NOW(), INTERVAL 21 DAY), 1001, DATE_SUB(NOW(), INTERVAL 20 DAY), 0),
  (4102, '携号转网办理条件与流程', 2031, 'SCRIPT', '携号转网资格查询、短信授权码获取与转入办理全流程。',
    '<h3>条件</h3><p>号码实名、无在途业务、无合约限制（或已到期）。</p><h3>流程</h3><p>发送授权码 → 携带证件到厅 → 新卡激活。</p>', 'ONLINE', 'MANUAL', 1003, 1, DATE_SUB(NOW(), INTERVAL 19 DAY), 478, 54, 1, 14, 5, 1003, DATE_SUB(NOW(), INTERVAL 20 DAY), 1003, DATE_SUB(NOW(), INTERVAL 19 DAY), 0),
  (4103, '宽带故障自助排查指南', 2012, 'SCRIPT', '用户报障前先自查：光猫指示灯、网线、重启步骤。',
    '<h3>指示灯</h3><p>PON 常亮为正常；LOS 闪红多为光纤问题。</p><h3>重启</h3><p>断电 30 秒后重启光猫与路由器。</p>', 'ONLINE', 'MANUAL', 1004, 1, DATE_SUB(NOW(), INTERVAL 18 DAY), 612, 81, 3, 22, 8, 1004, DATE_SUB(NOW(), INTERVAL 19 DAY), 1004, DATE_SUB(NOW(), INTERVAL 18 DAY), 0),
  (4104, '融合套餐家庭共享说明', 2021, 'PRODUCT', '融合套餐内流量/语音共享规则、副卡数量与资费。',
    '<h3>共享</h3><p>主卡套餐内流量可共享给最多 4 张副卡。</p><h3>副卡</h3><p>每张副卡月功能费 10 元，共享主套餐资源。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 17 DAY), 389, 42, 0, 11, 4, 1001, DATE_SUB(NOW(), INTERVAL 18 DAY), 1001, DATE_SUB(NOW(), INTERVAL 17 DAY), 0),
  (4105, '国际漫游开通与资费', 2022, 'PRODUCT', '出境前开通国际漫游、日包/月包选择与注意事项。',
    '<h3>开通</h3><p>APP 或营业厅均可开通，建议出境前 24 小时办理。</p><h3>资费</h3><p>热门国家日包 25 元起，超出按标准资费。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 16 DAY), 267, 31, 1, 9, 3, 1001, DATE_SUB(NOW(), INTERVAL 17 DAY), 1001, DATE_SUB(NOW(), INTERVAL 16 DAY), 0),
  (4106, '实名认证补登记操作指引', 2031, 'SCRIPT', '非实名或信息不全号码的补登记渠道与所需材料。',
    '<h3>材料</h3><p>机主身份证原件，代办需双方证件与授权书。</p><h3>渠道</h3><p>营业厅、官方 APP 实名专区。</p>', 'ONLINE', 'MANUAL', 1003, 1, DATE_SUB(NOW(), INTERVAL 15 DAY), 198, 22, 0, 6, 2, 1003, DATE_SUB(NOW(), INTERVAL 16 DAY), 1003, DATE_SUB(NOW(), INTERVAL 15 DAY), 0),
  (4107, '电子发票申请与重开', 2005, 'OFFICE', '个人/企业电子发票申请、抬头修改与重开规则。',
    '<h3>申请</h3><p>登录网上营业厅 → 发票管理 → 按账期开具。</p><h3>重开</h3><p>抬头错误可在 30 天内申请红冲重开一次。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 14 DAY), 341, 38, 2, 13, 4, 1001, DATE_SUB(NOW(), INTERVAL 15 DAY), 1001, DATE_SUB(NOW(), INTERVAL 14 DAY), 0),
  (4108, '积分兑换与清零规则', 2022, 'PRODUCT', '积分获取途径、兑换商城使用与年底清零政策说明。',
    '<h3>获取</h3><p>消费 1 元积 1 分，活动可加倍。</p><h3>清零</h3><p>年底未使用积分按规则结转或清零，以公告为准。</p>', 'ONLINE', 'MANUAL', 1002, 1, DATE_SUB(NOW(), INTERVAL 13 DAY), 456, 49, 1, 16, 5, 1002, DATE_SUB(NOW(), INTERVAL 14 DAY), 1002, DATE_SUB(NOW(), INTERVAL 13 DAY), 0),
  (4109, '校园套餐办理与续费', 2021, 'PRODUCT', '在校学生专属套餐资格、证件要求与毕业后续费方案。',
    '<h3>资格</h3><p>需提供学生证或在读证明，年龄 16–26 岁。</p><h3>续费</h3><p>毕业可转标准套餐或保留优惠至合约期满。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 12 DAY), 312, 35, 0, 10, 3, 1001, DATE_SUB(NOW(), INTERVAL 13 DAY), 1001, DATE_SUB(NOW(), INTERVAL 12 DAY), 0),
  (4110, '政企专线报障与 SLA', 2022, 'PRODUCT', '政企客户专线故障报障渠道、响应时限与升级机制。',
    '<h3>报障</h3><p>专属客户经理或 10009 政企热线。</p><h3>SLA</h3><p>AAA 级 4 小时修复，一般级 8 小时。</p>', 'ONLINE', 'MANUAL', 1002, 1, DATE_SUB(NOW(), INTERVAL 11 DAY), 156, 18, 0, 5, 2, 1002, DATE_SUB(NOW(), INTERVAL 12 DAY), 1002, DATE_SUB(NOW(), INTERVAL 11 DAY), 0),
  (4111, '物联网卡开卡与流量池', 2022, 'PRODUCT', '物联网卡适用场景、开卡流程与流量池共享计费。',
    '<h3>场景</h3><p>车联网、智能表计、监控设备等。</p><h3>流量池</h3><p>多卡共享流量包，按实际用量计费。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 10 DAY), 189, 21, 1, 7, 2, 1001, DATE_SUB(NOW(), INTERVAL 11 DAY), 1001, DATE_SUB(NOW(), INTERVAL 10 DAY), 0),
  (4112, '客户情绪安抚话术精选', 2013, 'SCRIPT', '面对愤怒、焦虑客户的共情话术与降调技巧。',
    '<h3>共情</h3><p>我非常理解您的心情，换作是我也会着急。</p><h3>降调</h3><p>语速放慢、音量降低，避免与客户抢话。</p>', 'ONLINE', 'MANUAL', 1004, 1, DATE_SUB(NOW(), INTERVAL 9 DAY), 724, 92, 4, 25, 9, 1004, DATE_SUB(NOW(), INTERVAL 10 DAY), 1004, DATE_SUB(NOW(), INTERVAL 9 DAY), 0),
  (4113, '欠费停机与复机流程', 2032, 'SCRIPT', '预付费/后付费欠费停机规则、复机方式与滞纳金说明。',
    '<h3>停机</h3><p>后付费欠费 T+3 停机，预付费余额为 0 即停。</p><h3>复机</h3><p>缴清欠费后 30 分钟内自动复机。</p>', 'ONLINE', 'MANUAL', 1003, 1, DATE_SUB(NOW(), INTERVAL 8 DAY), 403, 44, 2, 12, 4, 1003, DATE_SUB(NOW(), INTERVAL 9 DAY), 1003, DATE_SUB(NOW(), INTERVAL 8 DAY), 0),
  (4114, '副卡/add-on 业务办理', 2011, 'SCRIPT', '副卡、来电显示、彩铃等增值业务办理与退订话术。',
    '<h3>副卡</h3><p>主卡套餐允许下最多 4 张，需主卡授权。</p><h3>退订</h3><p>增值业务次月生效退订，当月费用不退。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 7 DAY), 278, 29, 0, 8, 3, 1001, DATE_SUB(NOW(), INTERVAL 8 DAY), 1001, DATE_SUB(NOW(), INTERVAL 7 DAY), 0),
  (4115, '营业厅排队取号指引', 2004, 'TRAIN', '引导客户使用线上取号、预约到店，减少等待时间。',
    '<h3>线上</h3><p>官方 APP 支持附近厅取号与排队提醒。</p><h3>到店</h3><p>扫码取号后留意叫号屏与短信通知。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 6 DAY), 145, 15, 0, 4, 1, 1001, DATE_SUB(NOW(), INTERVAL 7 DAY), 1001, DATE_SUB(NOW(), INTERVAL 6 DAY), 0),
  (4116, '千兆宽带提速营销话术', 2011, 'SCRIPT', '向老用户推荐千兆升级的话术、优惠与安装预约。',
    '<h3>卖点</h3><p>下载更快、多设备不卡、适合 4K 与游戏。</p><h3>优惠</h3><p>老用户升千兆可享 3 个月减免或设备升级。</p>', 'ONLINE', 'MANUAL', 1004, 1, DATE_SUB(NOW(), INTERVAL 5 DAY), 567, 71, 2, 19, 6, 1004, DATE_SUB(NOW(), INTERVAL 6 DAY), 1004, DATE_SUB(NOW(), INTERVAL 5 DAY), 0),
  (4117, '家庭组网与 Wi-Fi 覆盖', 2022, 'PRODUCT', 'Mesh 组网、电力猫与 FTTR 方案对比与推荐。',
    '<h3>Mesh</h3><p>适合已布网线家庭，无缝漫游。</p><h3>FTTR</h3><p>光纤到房间，大户型首选。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 4 DAY), 423, 47, 1, 14, 5, 1001, DATE_SUB(NOW(), INTERVAL 5 DAY), 1001, DATE_SUB(NOW(), INTERVAL 4 DAY), 0),
  (4118, '老年人套餐与防诈提醒', 2012, 'SCRIPT', '银发套餐介绍及向老年客户进行防诈宣传的话术。',
    '<h3>套餐</h3><p>大字版账单、专属客服、流量适中资费低。</p><h3>防诈</h3><p>不点陌生链接、不透露验证码与银行卡号。</p>', 'ONLINE', 'MANUAL', 1003, 1, DATE_SUB(NOW(), INTERVAL 3 DAY), 298, 33, 0, 9, 3, 1003, DATE_SUB(NOW(), INTERVAL 4 DAY), 1003, DATE_SUB(NOW(), INTERVAL 3 DAY), 0),
  (4119, '客服质检扣分项解读', 2004, 'TRAIN', '常见质检扣分：禁语、未核身、超时挂机等问题说明。',
    '<h3>禁语</h3><p>「不知道」「不归我管」等一律扣分。</p><h3>核身</h3><p>涉及账户变更必须完成身份核验。</p>', 'ONLINE', 'MANUAL', 1002, 1, DATE_SUB(NOW(), INTERVAL 2 DAY), 167, 19, 0, 5, 2, 1002, DATE_SUB(NOW(), INTERVAL 3 DAY), 1002, DATE_SUB(NOW(), INTERVAL 2 DAY), 0),
  (4120, '双卡双待与 eSIM 办理', 2022, 'PRODUCT', '双卡终端、eSIM 开卡条件与线上办理步骤。',
    '<h3>双卡</h3><p>支持双卡双待机型可主副号并用。</p><h3>eSIM</h3><p>指定机型与套餐，APP 内扫码写卡。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 1 DAY), 234, 26, 1, 7, 2, 1001, DATE_SUB(NOW(), INTERVAL 2 DAY), 1001, DATE_SUB(NOW(), INTERVAL 1 DAY), 0),
  (4121, '流量超出提醒与封顶', 2021, 'PRODUCT', '流量用尽提醒短信、封顶保护与加餐包推荐。',
    '<h3>提醒</h3><p>80%/100% 阈值自动短信与 APP 推送。</p><h3>封顶</h3><p>可设日/月封顶，超出暂停上网。</p>', 'ONLINE', 'MANUAL', 1003, 1, DATE_SUB(NOW(), INTERVAL 22 DAY), 356, 40, 1, 11, 4, 1003, DATE_SUB(NOW(), INTERVAL 23 DAY), 1003, DATE_SUB(NOW(), INTERVAL 22 DAY), 0),
  (4122, '工单催办与升级话术', 2013, 'SCRIPT', '客户催进度时的标准回复、升级条件与时限承诺。',
    '<h3>催办</h3><p>已为您加急，工单号 XXX，预计 X 小时内回复。</p><h3>升级</h3><p>超时未解决可升级至值班主管。</p>', 'ONLINE', 'MANUAL', 1004, 1, DATE_SUB(NOW(), INTERVAL 21 DAY), 445, 51, 2, 13, 5, 1004, DATE_SUB(NOW(), INTERVAL 22 DAY), 1004, DATE_SUB(NOW(), INTERVAL 21 DAY), 0),
  (4123, 'IPTV 与 OTT 业务区别', 2022, 'PRODUCT', '电视业务两种形态的功能、资费与故障处理差异。',
    '<h3>IPTV</h3><p>专网传输，直播稳定，需机顶盒。</p><h3>OTT</h3><p>互联网电视，内容更丰富，依赖宽带。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 20 DAY), 212, 24, 0, 6, 2, 1001, DATE_SUB(NOW(), INTERVAL 21 DAY), 1001, DATE_SUB(NOW(), INTERVAL 20 DAY), 0),
  (4124, '节假日值班安排说明', 2005, 'OFFICE', '春节、国庆等长假坐席排班、备班与交接要求。',
    '<h3>排班</h3><p>提前 2 周公布，不得擅自换班。</p><h3>交接</h3><p>值班日志与未结工单必须交接清楚。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 19 DAY), 98, 11, 0, 3, 1, 1001, DATE_SUB(NOW(), INTERVAL 20 DAY), 1001, DATE_SUB(NOW(), INTERVAL 19 DAY), 0),
  (4125, '异地补换卡办理指南', 2031, 'SCRIPT', '异地补卡、换卡条件、费用与身份核验要求。',
    '<h3>条件</h3><p>实名号码，无欠费、无在途挂失争议。</p><h3>费用</h3><p>补卡 20 元，VIP 免费 2 次/年。</p>', 'ONLINE', 'MANUAL', 1003, 1, DATE_SUB(NOW(), INTERVAL 18 DAY), 287, 32, 1, 8, 3, 1003, DATE_SUB(NOW(), INTERVAL 19 DAY), 1003, DATE_SUB(NOW(), INTERVAL 18 DAY), 0),
  (4126, '客户资料变更（过户）', 2032, 'SCRIPT', '号码过户双方到场、证件与结清欠费要求。',
    '<h3>双方</h3><p>原机主与新机主均须到场并持身份证。</p><h3>欠费</h3><p>须结清全部欠费与违约金后办理。</p>', 'ONLINE', 'MANUAL', 1002, 1, DATE_SUB(NOW(), INTERVAL 17 DAY), 176, 20, 0, 5, 2, 1002, DATE_SUB(NOW(), INTERVAL 18 DAY), 1002, DATE_SUB(NOW(), INTERVAL 17 DAY), 0),
  (4127, '外呼营销合规要点', 2004, 'TRAIN', '外呼时段、拒访登记与营销话术合规要求。',
    '<h3>时段</h3><p>工作日 9:00–12:00、14:00–20:00，周末 10:00–18:00。</p><h3>拒访</h3><p>客户拒绝后 90 天内不得再次营销外呼。</p>', 'ONLINE', 'MANUAL', 1002, 1, DATE_SUB(NOW(), INTERVAL 16 DAY), 134, 14, 0, 4, 1, 1002, DATE_SUB(NOW(), INTERVAL 17 DAY), 1002, DATE_SUB(NOW(), INTERVAL 16 DAY), 0),
  (4128, '视频客服接入指引', 2011, 'SCRIPT', '引导客户使用视频客服的话术、适用场景与操作步骤。',
    '<h3>场景</h3><p>实名核验、复杂业务演示、老年客户辅导。</p><h3>操作</h3><p>APP 首页 → 视频客服 → 允许摄像头与麦克风。</p>', 'ONLINE', 'MANUAL', 1004, 1, DATE_SUB(NOW(), INTERVAL 15 DAY), 223, 25, 0, 6, 2, 1004, DATE_SUB(NOW(), INTERVAL 16 DAY), 1004, DATE_SUB(NOW(), INTERVAL 15 DAY), 0),
  (4129, '套餐降档挽留话术', 2012, 'SCRIPT', '客户要求降档时的需求挖掘、替代方案与挽留策略。',
    '<h3>挖掘</h3><p>了解降档原因：资费、流量用不完、还是服务问题？</p><h3>替代</h3><p>推荐更匹配档位的套餐或短期优惠。</p>', 'ONLINE', 'MANUAL', 1004, 1, DATE_SUB(NOW(), INTERVAL 14 DAY), 501, 58, 3, 17, 6, 1004, DATE_SUB(NOW(), INTERVAL 15 DAY), 1004, DATE_SUB(NOW(), INTERVAL 14 DAY), 0),
  (4130, '知识库搜索技巧（坐席版）', 2004, 'TRAIN', '提高知识检索效率的关键词选择与筛选技巧。',
    '<h3>关键词</h3><p>用业务名词而非口语，如「携转」优于「换运营商」。</p><h3>筛选</h3><p>按分类、标签缩小范围，优先看「高频」标签。</p>', 'ONLINE', 'MANUAL', 1001, 1, DATE_SUB(NOW(), INTERVAL 13 DAY), 388, 43, 1, 12, 4, 1001, DATE_SUB(NOW(), INTERVAL 14 DAY), 1001, DATE_SUB(NOW(), INTERVAL 13 DAY), 0);

-- -----------------------------------------------------------------------------
-- 3. 文章-标签（每篇 1–3 个，复用 demo 标签 3001–3022）
-- -----------------------------------------------------------------------------
INSERT INTO kb_article_tag (id, article_id, tag_id) VALUES
  (41101, 4101, 3011), (41102, 4101, 3001), (41103, 4101, 3009),
  (41104, 4102, 3014), (41105, 4102, 3007), (41106, 4102, 3001),
  (41107, 4103, 3012), (41108, 4103, 3017), (41109, 4103, 3010),
  (41110, 4104, 3013), (41111, 4104, 3006), (41112, 4104, 3001),
  (41113, 4105, 3019), (41114, 4105, 3006),
  (41115, 4106, 3015), (41116, 4106, 3007),
  (41117, 4107, 3016), (41118, 4107, 3001),
  (41119, 4108, 3018), (41120, 4108, 3006),
  (41121, 4109, 3022), (41122, 4109, 3006), (41123, 4109, 3002),
  (41124, 4110, 3021), (41125, 4110, 3008),
  (41126, 4111, 3020), (41127, 4111, 3001),
  (41128, 4112, 3005), (41129, 4112, 3003), (41130, 4112, 3001),
  (41131, 4113, 3004), (41132, 4113, 3003),
  (41133, 4114, 3009), (41134, 4114, 3006),
  (41135, 4115, 3002), (41136, 4115, 3010),
  (41137, 4116, 3012), (41138, 4116, 3011), (41139, 4116, 3001),
  (41140, 4117, 3012), (41141, 4117, 3013),
  (41142, 4118, 3002), (41143, 4118, 3010),
  (41144, 4119, 3003), (41145, 4119, 3002),
  (41146, 4120, 3011), (41147, 4120, 3006),
  (41148, 4121, 3006), (41149, 4121, 3011), (41150, 4121, 3001),
  (41151, 4122, 3005), (41152, 4122, 3001),
  (41153, 4123, 3012), (41154, 4123, 3006),
  (41155, 4124, 3002),
  (41156, 4125, 3007), (41157, 4125, 3008),
  (41158, 4126, 3004), (41159, 4126, 3015),
  (41160, 4127, 3003), (41161, 4127, 3009),
  (41162, 4128, 3009), (41163, 4128, 3001),
  (41164, 4129, 3004), (41165, 4129, 3003), (41166, 4129, 3010),
  (41167, 4130, 3001), (41168, 4130, 3002), (41169, 4130, 3003);

-- -----------------------------------------------------------------------------
-- 4. 收藏（普通用户 + 部分坐席）
-- -----------------------------------------------------------------------------
INSERT INTO kb_favorite (id, article_id, user_id, folder, create_time) VALUES
  (42101, 4101, 1101, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 10 DAY)),
  (42102, 4101, 1103, '资费相关',   DATE_SUB(NOW(), INTERVAL 9 DAY)),
  (42103, 4102, 1102, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 8 DAY)),
  (42104, 4103, 1101, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (42105, 4103, 1104, '故障处理',   DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (42106, 4103, 1003, '常用话术',   DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (42107, 4108, 1103, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (42108, 4112, 1104, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (42109, 4112, 1110, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (42110, 4116, 1101, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (42111, 4116, 1106, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (42112, 4117, 1102, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (42113, 4121, 1102, '资费相关',   DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (42114, 4129, 1104, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (42115, 4130, 1108, '培训',       DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (42116, 4104, 1103, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (42117, 4105, 1109, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (42118, 4113, 1105, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (42119, 4122, 1004, '常用话术',   DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (42120, 4107, 1107, '默认收藏夹', DATE_SUB(NOW(), INTERVAL 2 DAY));

-- -----------------------------------------------------------------------------
-- 5. 评论（含部分二级回复）
-- -----------------------------------------------------------------------------
INSERT INTO kb_comment (id, article_id, user_id, parent_id, content, status, like_count, create_time, deleted) VALUES
  (44101, 4101, 1101, 0,     '5G 升级话术很清晰，照着念客户就懂了。', 'NORMAL', 4, DATE_SUB(NOW(), INTERVAL 8 DAY), 0),
  (44102, 4101, 1104, 44101, '我们厅里也在用，建议加一句流量结转说明。', 'NORMAL', 2, DATE_SUB(NOW(), INTERVAL 7 DAY), 0),
  (44103, 4101, 1001, 44102, '好建议，下版修订会补上。', 'NORMAL', 1, DATE_SUB(NOW(), INTERVAL 7 DAY), 0),
  (44104, 4102, 1102, 0,     '携转流程图要是能再详细一点就好了。', 'NORMAL', 3, DATE_SUB(NOW(), INTERVAL 6 DAY), 0),
  (44105, 4103, 1105, 0,     '光猫重启那步帮了大忙，很多用户自己就能搞定。', 'NORMAL', 6, DATE_SUB(NOW(), INTERVAL 6 DAY), 0),
  (44106, 4103, 1106, 0,     'LOS 闪红那段能否加张示意图？', 'NORMAL', 1, DATE_SUB(NOW(), INTERVAL 5 DAY), 0),
  (44107, 4104, 1103, 0,     '副卡共享规则终于搞明白了。', 'NORMAL', 2, DATE_SUB(NOW(), INTERVAL 5 DAY), 0),
  (44108, 4108, 1110, 0,     '积分年底清零之前记得提醒客户兑换。', 'NORMAL', 5, DATE_SUB(NOW(), INTERVAL 4 DAY), 0),
  (44109, 4112, 1104, 0,     '情绪安抚话术必背，投诉少很多。', 'NORMAL', 8, DATE_SUB(NOW(), INTERVAL 4 DAY), 0),
  (44110, 4112, 1101, 44109, '同感，共情那句特别管用。', 'NORMAL', 3, DATE_SUB(NOW(), INTERVAL 3 DAY), 0),
  (44111, 4112, 1006, 0,     '作为普通用户也觉得写得很好。', 'NORMAL', 2, DATE_SUB(NOW(), INTERVAL 3 DAY), 0),
  (44112, 4113, 1102, 0,     '欠费复机时间写得很明确。', 'NORMAL', 1, DATE_SUB(NOW(), INTERVAL 3 DAY), 0),
  (44113, 4116, 1101, 0,     '千兆营销话术成单率不错。', 'NORMAL', 4, DATE_SUB(NOW(), INTERVAL 2 DAY), 0),
  (44114, 4117, 1108, 0,     'FTTR 和 Mesh 对比很直观。', 'NORMAL', 2, DATE_SUB(NOW(), INTERVAL 2 DAY), 0),
  (44115, 4121, 1102, 0,     '流量封顶功能很多客户不知道，值得推广。', 'NORMAL', 3, DATE_SUB(NOW(), INTERVAL 2 DAY), 0),
  (44116, 4122, 1003, 0,     '催办话术里的时限承诺要注意别过度承诺。', 'NORMAL', 2, DATE_SUB(NOW(), INTERVAL 1 DAY), 0),
  (44117, 4129, 1104, 0,     '降档挽留的案例能不能多补几个？', 'NORMAL', 1, DATE_SUB(NOW(), INTERVAL 1 DAY), 0),
  (44118, 4129, 1004, 44117, '下一版会加三个真实案例。', 'NORMAL', 0, DATE_SUB(NOW(), INTERVAL 1 DAY), 0),
  (44119, 4130, 1108, 0,     '搜索技巧对新人太有用了。', 'NORMAL', 4, DATE_SUB(NOW(), INTERVAL 1 DAY), 0),
  (44120, 4105, 1109, 0,     '漫游日包价格写清楚了，出国前查这个就行。', 'NORMAL', 1, DATE_SUB(NOW(), INTERVAL 5 DAY), 0),
  (44121, 4107, 1107, 0,     '发票重开规则以前老搞混，现在清楚了。', 'NORMAL', 2, DATE_SUB(NOW(), INTERVAL 4 DAY), 0),
  (44122, 4118, 1105, 0,     '防诈提醒话术很重要，老年客户尤其需要。', 'NORMAL', 3, DATE_SUB(NOW(), INTERVAL 3 DAY), 0),
  (44123, 4125, 1106, 0,     '异地补卡 VIP 免费次数这个点不错。', 'NORMAL', 1, DATE_SUB(NOW(), INTERVAL 2 DAY), 0);

-- -----------------------------------------------------------------------------
-- 6. 点赞 / 点踩（文章 + 评论）
-- -----------------------------------------------------------------------------
INSERT INTO kb_like (id, target_type, target_id, user_id, type, create_time) VALUES
  (45101, 'ARTICLE', 4101, 1101, 1, DATE_SUB(NOW(), INTERVAL 9 DAY)),
  (45102, 'ARTICLE', 4101, 1102, 1, DATE_SUB(NOW(), INTERVAL 8 DAY)),
  (45103, 'ARTICLE', 4101, 1110, 1, DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (45104, 'ARTICLE', 4101, 1006, 1, DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (45105, 'ARTICLE', 4102, 1102, 1, DATE_SUB(NOW(), INTERVAL 8 DAY)),
  (45106, 'ARTICLE', 4102, 1103, 1, DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (45107, 'ARTICLE', 4103, 1101, 1, DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (45108, 'ARTICLE', 4103, 1104, 1, DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (45109, 'ARTICLE', 4103, 1105, 1, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45110, 'ARTICLE', 4103, 1003, 1, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45111, 'ARTICLE', 4103, 1004, 1, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (45112, 'ARTICLE', 4104, 1103, 1, DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (45113, 'ARTICLE', 4108, 1103, 1, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45114, 'ARTICLE', 4108, 1110, 1, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (45115, 'ARTICLE', 4112, 1101, 1, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45116, 'ARTICLE', 4112, 1104, 1, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (45117, 'ARTICLE', 4112, 1110, 1, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (45118, 'ARTICLE', 4112, 1006, 1, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (45119, 'ARTICLE', 4112, 1108, 1, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (45120, 'ARTICLE', 4116, 1101, 1, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (45121, 'ARTICLE', 4116, 1106, 1, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (45122, 'ARTICLE', 4117, 1102, 1, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (45123, 'ARTICLE', 4121, 1102, 1, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (45124, 'ARTICLE', 4129, 1104, 1, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (45125, 'ARTICLE', 4129, 1101, 1, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (45126, 'ARTICLE', 4130, 1108, 1, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (45127, 'ARTICLE', 4105, 1109, 1, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45128, 'ARTICLE', 4107, 1107, 1, DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (45129, 'ARTICLE', 4113, 1105, -1, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (45130, 'ARTICLE', 4122, 1004, 1, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (45131, 'COMMENT', 44101, 1102, 1, DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (45132, 'COMMENT', 44101, 1110, 1, DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (45133, 'COMMENT', 44105, 1101, 1, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45134, 'COMMENT', 44105, 1106, 1, DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (45135, 'COMMENT', 44109, 1101, 1, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (45136, 'COMMENT', 44109, 1108, 1, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (45137, 'COMMENT', 44109, 1110, 1, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (45138, 'COMMENT', 44119, 1101, 1, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (45139, 'COMMENT', 44119, 1104, 1, DATE_SUB(NOW(), INTERVAL 1 DAY));

-- -----------------------------------------------------------------------------
-- 7. 浏览记录（近 14 天，多用户、多文章）
-- -----------------------------------------------------------------------------
INSERT INTO kb_view_log (id, article_id, user_id, client_ip, stay_seconds, create_time) VALUES
  (46101, 4101, 1101, '192.168.2.11', 120, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46102, 4101, 1102, '192.168.2.12', 85,  DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46103, 4101, 1110, '192.168.2.20', 95,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46104, 4102, 1102, '192.168.2.12', 150, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46105, 4102, 1103, '192.168.2.13', 70,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46106, 4103, 1101, '192.168.2.11', 200, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46107, 4103, 1104, '192.168.2.14', 180, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46108, 4103, 1105, '192.168.2.15', 90,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46109, 4103, 1003, '192.168.1.23', 110, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46110, 4104, 1103, '192.168.2.13', 65,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46111, 4105, 1109, '192.168.2.19', 55,  DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46112, 4108, 1103, '192.168.2.13', 130, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46113, 4108, 1110, '192.168.2.20', 88,  DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46114, 4112, 1101, '192.168.2.11', 240, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46115, 4112, 1104, '192.168.2.14', 190, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46116, 4112, 1108, '192.168.2.18', 160, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46117, 4112, 1006, '192.168.1.26', 75,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46118, 4116, 1101, '192.168.2.11', 105, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46119, 4116, 1106, '192.168.2.16', 92,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46120, 4117, 1102, '192.168.2.12', 140, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46121, 4121, 1102, '192.168.2.12', 78,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46122, 4129, 1104, '192.168.2.14', 115, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46123, 4130, 1108, '192.168.2.18', 170, DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (46124, 4101, 1106, '192.168.2.16', 60,  DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (46125, 4103, 1106, '192.168.2.16', 45,  DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (46126, 4113, 1102, '192.168.2.12', 82,  DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46127, 4118, 1105, '192.168.2.15', 98,  DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46128, 4122, 1003, '192.168.1.23', 120, DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46129, 4125, 1106, '192.168.2.16', 70,  DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (46130, 4107, 1107, '192.168.2.17', 55,  DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (46131, 4109, 1108, '192.168.2.18', 88,  DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (46132, 4114, 1109, '192.168.2.19', 42,  DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (46133, 4120, 1101, '192.168.2.11', 66,  DATE_SUB(NOW(), INTERVAL 6 DAY)),
  (46134, 4123, 1103, '192.168.2.13', 54,  DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (46135, 4128, 1104, '192.168.2.14', 73,  DATE_SUB(NOW(), INTERVAL 7 DAY)),
  (46136, 4112, 1110, '192.168.2.20', 155, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46137, 4116, 1004, '192.168.1.24', 99,  DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (46138, 4130, 1101, '192.168.2.11', 110, DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (46139, 4102, 1106, '192.168.2.16', 67,  DATE_SUB(NOW(), INTERVAL 5 DAY)),
  (46140, 4104, 1107, '192.168.2.17', 58,  DATE_SUB(NOW(), INTERVAL 4 DAY));

-- -----------------------------------------------------------------------------
-- 8. 搜索日志
-- -----------------------------------------------------------------------------
INSERT INTO kb_search_log (id, user_id, keyword, result_count, client_ip, create_time) VALUES
  (47101, 1101, '5G',       4, '192.168.2.11', DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (47102, 1102, '携号转网', 2, '192.168.2.12', DATE_SUB(NOW(), INTERVAL 1 DAY)),
  (47103, 1103, '积分',     3, '192.168.2.13', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (47104, 1104, '情绪安抚', 2, '192.168.2.14', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (47105, 1106, '宽带故障', 3, '192.168.2.16', DATE_SUB(NOW(), INTERVAL 2 DAY)),
  (47106, 1108, '搜索技巧', 1, '192.168.2.18', DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (47107, 1110, '千兆',     2, '192.168.2.20', DATE_SUB(NOW(), INTERVAL 3 DAY)),
  (47108, 1105, '漫游',     1, '192.168.2.15', DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (47109, 1109, '发票',     2, '192.168.2.19', DATE_SUB(NOW(), INTERVAL 4 DAY)),
  (47110, 1107, '物联网',   1, '192.168.2.17', DATE_SUB(NOW(), INTERVAL 5 DAY));

-- -----------------------------------------------------------------------------
-- 9. 近 5 天日统计（热门文章样本）
-- -----------------------------------------------------------------------------
INSERT INTO kb_usage_stat (id, article_id, stat_date, view_count, like_count, favorite_count, share_count, comment_count, download_count, create_time, update_time) VALUES
  (48101, 4101, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 48, 6, 2, 0, 1, 0, NOW(), NOW()),
  (48102, 4101, DATE_SUB(CURDATE(), INTERVAL 2 DAY), 52, 7, 1, 0, 0, 0, NOW(), NOW()),
  (48103, 4103, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 61, 8, 3, 0, 2, 0, NOW(), NOW()),
  (48104, 4103, DATE_SUB(CURDATE(), INTERVAL 2 DAY), 55, 6, 2, 0, 1, 0, NOW(), NOW()),
  (48105, 4112, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 72, 9, 2, 0, 2, 0, NOW(), NOW()),
  (48106, 4112, DATE_SUB(CURDATE(), INTERVAL 2 DAY), 68, 8, 2, 0, 1, 0, NOW(), NOW()),
  (48107, 4116, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 44, 5, 2, 0, 1, 0, NOW(), NOW()),
  (48108, 4129, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 39, 4, 1, 0, 1, 0, NOW(), NOW()),
  (48109, 4130, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 35, 4, 1, 0, 1, 0, NOW(), NOW()),
  (48110, 4102, DATE_SUB(CURDATE(), INTERVAL 1 DAY), 41, 5, 1, 0, 0, 0, NOW(), NOW());

COMMIT;

-- =============================================================================
-- 补充完成。新增账号 user2–user11（密码 123456），30 条 ONLINE 知识 4101–4130。
-- 可重复执行；仅清理/覆盖本脚本 ID 段，不影响 demo_data.sql 原有数据。
-- =============================================================================
