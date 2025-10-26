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
#include <QPlainTextEdit>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QFuture>
#include <QFutureWatcher>
#include <QProgressDialog>

#include "outfilegenerator.h"
#include "jsonprofilemanager.h"
#include "comboboxdelegate.h"
#include "dynamiccomboboxdelegate.h"

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
    void setupTable(QTableWidget*);
    void addPlusRow(QTableWidget*);
    void fillTable(const QJsonArray& data, const QStringList& mainKeys, QTableWidget* table, bool configRow = false);
    

    void processError(const QString& message, const QString& title);
    bool saveProfileHandler(bool isSaveAs);
    void deleteRow();
    void addRow();
    void setRowType(const QString& type, int rowIndex, QTableWidget* table, const QVector<QString>& data = {}, const QString& accessType = "R");
    void showContextMenuForTable(const QPoint &pos, QTableWidget* table);

    //Обработка долгих функций
    QFutureWatcher<std::optional<JsonProfileManager::TProfileResult>>* profileWatcher;
    QFutureWatcher<bool>* fileGenWatcher;
    QProgressDialog* progressDialog;

    // Статическая функция для загрузки профиля в фоновом потоке
    static std::optional<JsonProfileManager::TProfileResult> loadProfileInBackground(JsonProfileManager* manager);
    // Статическая функция для генерации файла в фоновом потоке
    static std::optional<bool> genFileInBackground(OutFileGenerator* generator);
    
    int contextMenuClickRow = -1;
    QTableWidget* contextMenuActiveTable = nullptr;
    
    // Делегаты для комбобоксов
    ComboBoxDelegate *accessTypeDelegate;
    ComboBoxDelegate *dataTypeDelegate;
    DynamicComboBoxDelegate *baseValueDelegate;
    ComboBoxDelegate *iqFormatDelegate;

    typedef enum  {
        R = 0, //Чтение
        W = 1, //Запись
    } TaccessVarieties;

    //Названия колонок таблицы с данными
    const QStringList TABLE_HEADERS = {
        "Название группы параметров / параметра",
        "Тип доступа", "Тип данных", "Коэффициент",
        "Адрес (дес.)", "Адрес (hex.)","Переменная / значение","Базовая величина","Примечание"
    };

    //Названия колонок таблицы с базовыми величинами
    const QStringList BASE_VALUES_HEADERS = {
        "Название базовой величины", "Единицы",
        "Формат IQ", "Переменная / значение", "Примечание"
    };


private slots:
    void onSelectionChanged();
    void onCellClickedData(int row, int col);
    void onCellDoubleClickedData(int row, int col);
    void onCellClickedBaseValues(int row, int col);
    bool loadProfile();
    bool onProfileLoadFinished();
    bool saveProfile();
    bool saveProfileAs();
    bool startGeneration();
    bool onGenerationFinished();
    bool setGenerationPath();
    void showContextMenu(const QPoint &pos);
    void showContextMenuBaseValues(const QPoint &pos);
    void onBaseValuesChanged();
};
#endif // MAINWINDOW_H
