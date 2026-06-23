#include "app/Application.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>

static void applyLightPalette(QApplication &app) {
    QPalette pal;
    pal.setColor(QPalette::Window, QColor("#F4F6F9"));
    pal.setColor(QPalette::WindowText, QColor("#1F2A37"));
    pal.setColor(QPalette::Base, QColor("#FFFFFF"));
    pal.setColor(QPalette::AlternateBase, QColor("#FAFBFC"));
    pal.setColor(QPalette::Text, QColor("#1F2A37"));
    pal.setColor(QPalette::Button, QColor("#FFFFFF"));
    pal.setColor(QPalette::ButtonText, QColor("#1F2A37"));
    pal.setColor(QPalette::Highlight, QColor("#2563EB"));
    pal.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
    pal.setColor(QPalette::PlaceholderText, QColor("#9098A3"));
    app.setPalette(pal);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("kb");
    QApplication::setApplicationName("knowledgeAnswer");
    QApplication::setApplicationDisplayName(QStringLiteral("智能客服知识库系统"));

    QApplication::setStyle(QStyleFactory::create("Fusion"));
    applyLightPalette(app);

    QFile qss(":/styles/app.qss");
    if (qss.open(QFile::ReadOnly | QFile::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
    }

    kb::Application controller;
    if (!controller.start()) {
        return 0;
    }
    return QApplication::exec();
}
