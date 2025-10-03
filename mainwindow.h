#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

#include "outfilegenerator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    OutFileGenerator outFileGenerator;
    void setupUI();
    void addPlusRow();

    QString activeProfilePath = ""; //Путь до активного профиля
    
    void showProfileError(const QString& message, const QString& title = "Ошибка профиля");

    //Ключи json файла
    const QStringList COLUMN_KEYS = {"groupName", "accessType", "dataType", "gain",
                                     "units", "range", "addressDec", "addressHex", "note"};

    //Названия колонок таблицы
    const QStringList TABLE_HEADERS = {
        "Название группы параметров / параметра",
        "Тип доступа", "Тип данных", "Коэффициент",
        "Ед. изм.", "Диапазон значений",
        "Адрес (дес.)", "Адрес (hex.)", "Примечание"
    };


private slots:
    void onSelectionChanged();
    void onCellClicked(int row, int col);
    bool readProfile();
    bool saveProfile();
    bool saveAsProfile();
};
#endif // MAINWINDOW_H
