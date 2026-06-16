#include "app/Application.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("kb");
    QApplication::setApplicationName("knowledgeAnswer");
    QApplication::setApplicationDisplayName(QStringLiteral("智能客服知识库系统"));

    // 全局主题
    QFile qss(":/styles/app.qss");
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
    }

    kb::Application controller;
    if (!controller.start()) {
        return 0; // 用户取消登录
    }
    return QApplication::exec();
}
