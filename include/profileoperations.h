#ifndef PROFILEOPERATIONS_H
#define PROFILEOPERATIONS_H

#include <QObject>
#include <QWidget>
#include <QTableWidget>
#include <QProgressDialog>
#include <QFutureWatcher>
#include <QtConcurrent/qtconcurrentrun.h>
#include <optional>
#include "jsonprofilemanager.h"
#include "tablemanager.h"

// Класс для управления операциями с профилями (загрузка, сохранение)
class ProfileOperations : public QObject
{
    Q_OBJECT

public:
    explicit ProfileOperations(QWidget* parentWidget,
                              JsonProfileManager* profileManager,
                              TableManager* tableManager,
                              QObject *parent = nullptr);

    // Загрузить профиль
    void loadProfile(QTableWidget* dataTable, QTableWidget* baseValuesTable);

    // Сохранить профиль
    bool saveProfile(QTableWidget* dataTable, QTableWidget* baseValuesTable);

    // Сохранить профиль как...
    bool saveProfileAs(QTableWidget* dataTable, QTableWidget* baseValuesTable);

signals:
    // Сигнал об успешной загрузке профиля
    void profileLoaded(const QString& profilePath);

    // Сигнал об успешном сохранении профиля
    void profileSaved(const QString& profilePath);

    // Сигнал об ошибке
    void errorOccurred(const QString& message, const QString& title);

private slots:
    void onProfileLoadFinished();

private:
    // Статическая функция для загрузки профиля в фоновом потоке
    static std::optional<JsonProfileManager::TProfileResult> loadProfileInBackground(JsonProfileManager* manager);

    bool saveProfileHandler(QTableWidget* dataTable, QTableWidget* baseValuesTable, bool isSaveAs);

    QWidget* parentWidget;
    JsonProfileManager* profileManager;
    TableManager* tableManager;

    // Для асинхронной загрузки
    QFutureWatcher<std::optional<JsonProfileManager::TProfileResult>>* profileWatcher;
    QProgressDialog* progressDialog;

    // Таблицы (для использования в onProfileLoadFinished)
    QTableWidget* currentDataTable;
    QTableWidget* currentBaseValuesTable;
};

#endif // PROFILEOPERATIONS_H
