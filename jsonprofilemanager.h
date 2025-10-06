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

    // Основные операции с профилями
    std::optional<QJsonArray> loadProfile();// Диалог + загрузка
    bool saveProfile(QTableWidget* table);   // Сохранить в текущий файл
    bool saveProfileAs(QTableWidget* table); // Сохранить как...
    std::optional<QJsonArray> readJsonFile(const QString& filePath);
    std::optional<QJsonArray> readJsonFile() {return readJsonFile(currentProfilePath);};

    // Валидация
    bool validateProfile(const QJsonArray& data);

    // Геттеры
    QString getCurrentProfilePath() const { return currentProfilePath; }
    QString getLastError() const { return lastError; }
    bool hasActiveProfile() const { return !currentProfilePath.isEmpty(); }

    //Ключи json файла
    const QStringList COLUMN_KEYS = {"groupName", "accessType", "dataType", "gain",
                                     "units", "range", "addressDec", "addressHex", "note"};

private:
    QWidget* parent;
    QString  currentProfilePath;
    QString  lastError;

    // Внутренние методы
    bool writeJsonFile(const QString& filePath, const QJsonArray& data);
    void setError(const QString& error) { lastError = error; }

    // Работа с настройками
    QString getLastDirectory() const;
    void saveLastDirectory(const QString& path);
};

#endif // JSONPROFILEMANAGER_H
