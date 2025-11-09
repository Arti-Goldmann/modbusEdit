#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QFileInfo>
#include <QCloseEvent>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QtConcurrent/qtconcurrentrun.h>

#include "outfilegenerator.h"
#include "jsonprofilemanager.h"
#include "tablemanager.h"
#include "profileoperations.h"
#include "comboboxdelegate.h"
#include "dynamiccomboboxdelegate.h"
#include "constants.h"

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

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;

    // Менеджеры
    OutFileGenerator outFileGenerator;
    JsonProfileManager jsonProfileManager;
    TableManager* tableManager;
    ProfileOperations* profileOperations;

    // Делегаты для комбобоксов
    ComboBoxDelegate *accessTypeDelegate;
    ComboBoxDelegate *dataTypeDelegate;
    DynamicComboBoxDelegate *baseValueDelegate;
    ComboBoxDelegate *iqFormatDelegate;

    // Флаг несохраненных изменений
    bool hasUnsavedChanges = false;

    // Для асинхронной генерации файла
    QFutureWatcher<bool>* fileGenWatcher;
    QProgressDialog* progressDialog;

    // UI setup
    void setupUI();
    void setupDelegates();
    void connectSignals();

    // Вспомогательные функции
    void setModified(bool modified = true);
    bool maybeSave();
    void processError(const QString& message, const QString& title);

    // Статическая функция для генерации файла в фоновом потоке
    static bool genFileInBackground(OutFileGenerator* generator, QJsonArray data, QJsonArray baseValues);

private slots:
    // Слоты для меню
    void onLoadProfile();
    void onSaveProfile();
    void onSaveProfileAs();
    bool onSetGenerationPath();
    void onStartGeneration();

    // Слоты от TableManager
    void onDataModified();

    // Слоты от ProfileOperations
    void onProfileLoaded(const QString& profilePath);
    void onProfileSaved(const QString& profilePath);

    // Слоты для базовых величин
    void onBaseValuesChanged();

    // Слот для генерации
    void onGenerationFinished();
};

#endif // MAINWINDOW_H
