#pragma once

#include "app/RefreshablePage.h"

#include <QString>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QJsonObject;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace kb {

/**
 * AI 配置页（模块 9，路由 ai.config，权限码 ai:config，仅管理员）：
 *   - LLM 接入配置表单（预设模型下拉 / base_url / api_key / model / 温度 / max_tokens / 启用），可「测试连接」与「保存」
 *     · 顶部「预设模型」选择后自动填充接口地址与模型名；也可手动填写覆盖，下拉会回退到「自定义」
 *     · 保存经 Java 落库并实时下发 Python（无需重启）；api_key 留空＝保持原值，回显为掩码占位
 *   - 向量索引：「重建全部在线知识索引」（复用 ai:index，按权限启用）
 * 仅管理员能看到本菜单（V11 只授 ADMIN）；页首再做一次防御性 hasPermission 校验。
 */
class AiConfigPage : public QWidget, public RefreshablePage {
    Q_OBJECT

public:
    explicit AiConfigPage(const QString &title, QWidget *parent = nullptr);

    void refreshPage() override;

private:
    void buildUi();
    void loadConfig();
    void testConnection();
    void saveConfig();
    void rebuildIndex();

    QJsonObject collectBody() const;
    void setStatus(QLabel *label, const QString &text, int level = 0); // 0 info /1 ok /2 error

    // 预设模型下拉：选择时自动填充接口地址 + 模型名
    void populatePresets();
    void applyPreset(int index);
    void syncPresetFromFields(); // 根据当前 model/baseUrl 反查匹配的预设

    QString m_title;

    QComboBox *m_preset = nullptr;
    QLineEdit *m_baseUrl = nullptr;
    QLineEdit *m_apiKey = nullptr;
    QLineEdit *m_model = nullptr;
    QDoubleSpinBox *m_temperature = nullptr;
    QSpinBox *m_maxTokens = nullptr;
    QCheckBox *m_enabled = nullptr;
    QPushButton *m_testBtn = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QLabel *m_status = nullptr;

    QPushButton *m_rebuildBtn = nullptr;
    QLabel *m_rebuildStatus = nullptr;
};

} // namespace kb
