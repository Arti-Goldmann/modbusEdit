#ifndef OUTFILEGENERATOR_H
#define OUTFILEGENERATOR_H

#include <QFile>
#include <QString>
#include <QTextStream>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QHash>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>

class OutFileGenerator
{
public:
    OutFileGenerator(QWidget* parent);
    bool setGenerationPath();
    QString getError();
    QString getCurrentGenFilePath() const { return currentGenFilePath; }
    bool hasActiveGenFile() const { return !currentGenFilePath.isEmpty(); }

    bool generate(const QJsonArray& jsonArr, const QString& absGenPath);
private:
    QWidget* parent;
    QString lastError;
    QString currentGenFilePath;
    QString getLastDirectory() const;
    void saveLastDirectory(const QString& path);

    const QHash<QString, QString> TYPE_TO_IQ_FUNC = {
        {"int16",  "Int16toIQ"},
        {"Uint16", "UInt16toIQ"},
    };

    const QHash<QString, QString> IQ_TO_TYPE_FUNC = {
        {"int16", "IQtoInt16"},
        {"Uint16", "IQtoUInt16"},
        };

    QString funcHandlerGen(const QString& funcName, const QString&type, const QJsonArray& jsonArr, const QHash<QString, QString>& FUNC);
    QString arrayGen(const QString& arrName, const QString&type, const QJsonArray& jsonArr);
    void setError(const QString& message);
};

#endif // OUTFILEGENERATOR_H
