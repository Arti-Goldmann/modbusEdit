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
    bool saveProfile(QTableWidget* tableData, QTableWidget* tableBaseValues);   // Сохранить в текущий файл
    bool saveProfileAs(QTableWidget* tableData, QTableWidget* tableBaseValues); // Сохранить как...

    std::optional<QJsonArray> readJsonData(const QString& filePath);
    std::optional<QJsonArray> readJsonData() {return readJsonData(currentProfilePath);};
    std::optional<QJsonArray> readJsonBaseValues(const QString& filePath);
    std::optional<QJsonArray> readJsonBaseValues() {return readJsonBaseValues(currentProfilePath);};

    // Валидация
    bool validateProfile(const QJsonArray& data);

    // Геттеры
    QString getCurrentProfilePath() const { return currentProfilePath; }
    QString getLastError() const { return lastError; }
    bool hasActiveProfile() const { return !currentProfilePath.isEmpty(); }

    //Ключи json файла
    const QStringList COLUMN_KEYS = {"groupName", "accessType", "dataType", "gain",
                                     "units", "range", "addressDec", "addressHex", "note"};

    const QStringList BASE_VALUES_KEYS = {"baseName", "units", "IQformat", "baseValue", "note"};

private:
    QWidget* parent;
    QString  currentProfilePath;
    QString  lastError;

    // Внутренние методы
    bool writeJsonFile(const QString& filePath, const QJsonArray& data);
    void setError(const QString& error) { lastError = error; }
    std::optional<QJsonObject> readJsonObj(const QString& filePath);

    // Работа с настройками
    QString getLastDirectory() const;
    void saveLastDirectory(const QString& path);
};

#endif // JSONPROFILEMANAGER_H
