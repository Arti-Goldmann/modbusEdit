#ifndef JSONPROFILEMANAGER_H
#define JSONPROFILEMANAGER_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QWidget>
#include <optional>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QTableWidget>
#include <QWidget>

class JsonProfileManager
{
public:
    explicit JsonProfileManager(QWidget* parent);

    typedef struct {
        QJsonArray  data;
        QJsonArray  baseValues;
    } TProfileResult;

    // Основные операции с профилями
    std::optional<TProfileResult> loadProfile(); // Диалог + загрузка
    std::optional<TProfileResult> readProfile() {return readProfileFormPath(currentProfilePath);}; // загрузка
    bool saveProfile(QTableWidget* tableData, QTableWidget* tableBaseValues);   // Сохранить в текущий файл
    bool saveProfileAs(QTableWidget* tableData, QTableWidget* tableBaseValues); // Сохранить как...



    // Валидация
    bool validateProfile(const QJsonArray& data);

    // Геттеры
    QString getCurrentProfilePath() const { return currentProfilePath; }
    QString getLastError() const { return lastError; }
    bool hasActiveProfile() const { return !currentProfilePath.isEmpty(); }

    //Ключи json файла
    //Ключи параметров, которые соотвествуют колонкам таблицы
    const QStringList DATA_MAIN_KEYS = {"groupName", "accessType", "dataType", "gain",
                                     "addressDec", "addressHex", "varName", "base", "note"};

    //Ключи параметров, которые есть у строки, но в таблице не отображены
    const QStringList DATA_HIDDEN_KEYS = {"paramType", "userCode"};

    const QStringList BASE_VALUES_KEYS = {"baseName", "units", "IQformat", "baseValue", "note"};

private:
    QWidget* parent;
    QString  currentProfilePath;
    QString  lastError;

    // Внутренние методы
    bool writeJsonFile(const QString& filePath, const QJsonArray& data);
    void setError(const QString& error) { lastError = error; }
    std::optional<QJsonObject> readJsonObj(const QString& filePath);
    std::optional<QJsonArray> readJsonArr(const QString& filePath, const QString& field);
    std::optional<TProfileResult> readProfileFormPath(const QString& profilePath); // загрузка
    QJsonArray tableToJsonArray(QTableWidget* table, const QStringList&, const QStringList& = {});

    // Работа с настройками
    QString getLastDirectory() const;
    void saveLastDirectory(const QString& path);
};

#endif // JSONPROFILEMANAGER_H
