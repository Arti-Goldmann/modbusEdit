#ifndef EXCELCOLUMNSDIALOG_H
#define EXCELCOLUMNSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QVector>
#include "excelexchange.h"

// Диалог выбора столбцов (заголовков) для экспорта/импорта Excel.
// Отмеченные галочки запоминаются в QSettings отдельно для экспорта и импорта.
class ExcelColumnsDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Mode { Export, Import };

    // groups - группы параметров (заголовки), из которых можно выбрать нужные
    explicit ExcelColumnsDialog(Mode mode, const QStringList& groups, QWidget* parent = nullptr);

    QStringList selectedDataColumns() const { return selectedIds(dataBoxes); }
    QStringList selectedBaseColumns() const { return selectedIds(baseBoxes); }
    QStringList selectedGroups() const { return selectedIds(groupBoxes); }

    void accept() override;

private:
    struct Box {
        QString id;
        QCheckBox* checkBox;
    };

    Mode mode;
    QVector<Box> dataBoxes;
    QVector<Box> baseBoxes;
    QVector<Box> groupBoxes;
    QPushButton* okButton = nullptr;

    QWidget* createGroup(const QString& title, const QVector<ExcelExchange::Column>& columns,
                         const QString& settingsKey, QVector<Box>& boxes);
    QWidget* createGroupsBox(const QStringList& groups);
    static QStringList selectedIds(const QVector<Box>& boxes);
    static void setAll(const QVector<Box>& boxes, bool checked);
    void updateOkButton();
    QString settingsPrefix() const;
};

#endif // EXCELCOLUMNSDIALOG_H
