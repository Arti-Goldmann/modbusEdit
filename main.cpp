#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QApplication::setStyle("Fusion");  // Использовать стиль Fusion вместо системного
    return a.exec();
}
