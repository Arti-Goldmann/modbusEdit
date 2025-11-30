#ifndef TABLEMANAGER_H
#define TABLEMANAGER_H

#include <QObject>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QMenu>
#include <QPoint>
#include <QJsonArray>
#include <QJsonObject>
#include <QVector>
#include <QApplication>
#include <QBrush>
#include <QFont>
#include "constants.h"


// Класс для управления таблицами
class TableManager : public QObject
{
    Q_OBJECT

public:
    explicit TableManager(QObject *parent = nullptr, QTableWidget* dataTable = nullptr, QTableWidget* baseValuesTable = nullptr);

    // Базовая настройка таблицы
    void setupTable(QTableWidget* table);

    // Добавить последнюю строку с плюсиком
    void addPlusRow(QTableWidget* table);

    // Заполнить таблицу данными из JSON
    void fillTable(QTableWidget* table,
                   const QJsonArray& data,
                   const QStringList& mainKeys,
                   bool configRow = false);

    // Установить тип строки (commonType или userType)
    void setRowType(const QString& rowType,
                   int rowIndex,
                   QTableWidget* table,
                   const QVector<QString>& data = {},
                   const QString& accessType = Constants::AccessType::READ_ONLY);

    // Обновить состояние ячеек в зависимости от типа данных привода
    void updateCellsStateByDataType(int rowIndex, QTableWidget* table);

    // Обработчики событий таблицы
    void handleCellClicked(QTableWidget* table, int row, int col);
    void handleCellDoubleClicked(QTableWidget* table, int row, int col);
    void handleTableDataChanged(QTableWidget* table, QTableWidgetItem* item);
    void handleContextMenu(QTableWidget* table, const QPoint &pos);

signals:
    // Сигнал об изменении данных в таблице
    void dataModified();

    // Сигнал об ошибке
    void errorOccurred(const QString& message, const QString& title);

private:
    // Контекстное меню
    void showContextMenuForTable(const QPoint &pos, QTableWidget* table);
    void deleteRow();
    void addRow();
    void renumberAddresses(int titleRow);

    // Текущая таблица и строка для контекстного меню
    int contextMenuClickRow = -1;
    QTableWidget* contextMenuActiveTable = nullptr;
    QTableWidget* dataTable = nullptr;
    QTableWidget* baseValuesTable = nullptr;
};

#endif // TABLEMANAGER_H
