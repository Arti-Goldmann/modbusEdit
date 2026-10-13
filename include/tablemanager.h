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
#include <functional>
#include "constants.h"


// Класс для управления таблицами
class TableManager : public QObject
{
    Q_OBJECT

public:
    explicit TableManager(QObject *parent = nullptr, QTableWidget* dataTable = nullptr, QTableWidget* baseValuesTable = nullptr);
    ~TableManager() override;

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

    // Горячие клавиши для работы со строками таблицы (Del, Insert, Ctrl+D, Ctrl+Shift+D)
    void setupShortcuts(QTableWidget* table);

    // Операции со строками
    void deleteRow(QTableWidget* table, int row);           // Для заголовка спрашивает, удалять ли параметры группы
    void addRow(QTableWidget* table, int row);              // Добавляет строку после row
    void copyRow(QTableWidget* table, int row, bool incrementAddress); // Копия строки ниже исходной
    void copyGroup(QTableWidget* table, int titleRow);      // Копия заголовка вместе с параметрами группы
    void cutRow(QTableWidget* table, int row);              // Вырезать строку (для заголовка спрашивает про параметры)
    void pasteRows(QTableWidget* table, int row);           // Вставить вырезанное над строкой row
    bool canPaste(QTableWidget* table) const { return table && table == cutTable && !cutBuffer.isEmpty(); }

signals:
    // Сигнал об изменении данных в таблице
    void dataModified();

    // Сигнал об ошибке
    void errorOccurred(const QString& message, const QString& title);

private:
    // Контекстное меню
    void showContextMenuForTable(const QPoint &pos, QTableWidget* table);
    void applyDefaultComboValues(int rowIndex, QTableWidget* table);
    void renumberAddresses(int titleRow);
    void deleteTitleWithParams(QTableWidget* table, int titleRow);

    // Вспомогательные функции
    bool isDataRow(QTableWidget* table, int row) const;     // Строка с данными (не строка с плюсиком)
    QString rowTypeOf(QTableWidget* table, int row) const;
    int groupEndRow(QTableWidget* table, int titleRow) const; // Первая строка после группы заголовка
    void cloneRow(QTableWidget* table, int srcRow, int dstRow);
    void shiftAddress(QTableWidget* table, int row, int delta);
    void sortGroupByAddress(QTableWidget* table, QTableWidgetItem* changedItem); // Сортировка строк группы по адресу

    // Текущая таблица и строка для контекстного меню
    int contextMenuClickRow = -1;
    QTableWidget* contextMenuActiveTable = nullptr;
    QTableWidget* dataTable = nullptr;
    QTableWidget* baseValuesTable = nullptr;

    // Буфер вырезанных строк: копии ячеек каждой строки (вместе со скрытыми данными)
    QVector<QVector<QTableWidgetItem*>> cutBuffer;
    QTableWidget* cutTable = nullptr; // Из какой таблицы вырезали - вставлять можно только туда же
    void clearCutBuffer();
};

#endif // TABLEMANAGER_H
