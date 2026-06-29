#include "app/Application.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>

static void applyLightPalette(QApplication &app) {
    QPalette pal;
    pal.setColor(QPalette::Window, QColor("#FEFEFE"));
    pal.setColor(QPalette::WindowText, QColor("#000000"));
    pal.setColor(QPalette::Base, QColor("#FEFEFE"));
    pal.setColor(QPalette::AlternateBase, QColor("#F3EDE4"));
    pal.setColor(QPalette::Text, QColor("#000000"));
    pal.setColor(QPalette::Button, QColor("#F1EEE8"));
    pal.setColor(QPalette::ButtonText, QColor("#000000"));
    pal.setColor(QPalette::Highlight, QColor("#F0EBE4"));
    pal.setColor(QPalette::HighlightedText, QColor("#000000"));
    pal.setColor(QPalette::PlaceholderText, QColor("#757575"));
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
