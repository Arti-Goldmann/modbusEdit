#include "mainwindow.h"

#include <QApplication>
#include <QPalette>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QApplication::setStyle("Fusion");  // Использовать стиль Fusion вместо системного

    // Настраиваем тёмную светлую палитру с контрастными строками
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(200, 200, 200));
    lightPalette.setColor(QPalette::WindowText, Qt::black);
    lightPalette.setColor(QPalette::Base, QColor(245, 245, 245));          // Светлые строки
    lightPalette.setColor(QPalette::AlternateBase, QColor(220, 220, 220)); // Темные строки - больше контраст
    lightPalette.setColor(QPalette::ToolTipBase, QColor(230, 230, 230));
    lightPalette.setColor(QPalette::ToolTipText, Qt::black);
    lightPalette.setColor(QPalette::Text, Qt::black);
    lightPalette.setColor(QPalette::Button, QColor(200, 200, 200));
    lightPalette.setColor(QPalette::ButtonText, Qt::black);
    lightPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    lightPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    lightPalette.setColor(QPalette::HighlightedText, Qt::white);
    lightPalette.setColor(QPalette::Disabled, QPalette::Base, QColor(195, 195, 195));
    lightPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(90, 90, 90));

    a.setPalette(lightPalette);

    MainWindow w;
    w.show();

    return a.exec();
}
