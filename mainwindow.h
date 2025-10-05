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
#include "jsonprofilemanager.h"

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
    JsonProfileManager jsonProfileManager;
    void setupUI();
    void addPlusRow();
    
    void processError(const QString& message, const QString& title);
    bool saveProfileHandler(bool isSaveAs);

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
    bool loadProfile();
    bool saveProfile();
    bool saveProfileAs();
    bool startGeneration();
    bool setGenerationPath();
};
#endif // MAINWINDOW_H
