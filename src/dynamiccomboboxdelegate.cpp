#include "dynamiccomboboxdelegate.h"
#include <QComboBox>
#include <QTableWidgetItem>

DynamicComboBoxDelegate::DynamicComboBoxDelegate(QTableWidget *sourceTable, int sourceColumn, QObject *parent)
    : QStyledItemDelegate(parent), m_sourceTable(sourceTable), m_sourceColumn(sourceColumn)
{
}

QWidget *DynamicComboBoxDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem &option,
                                              const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    
    QComboBox *comboBox = new QComboBox(parent);
    
    // Динамически заполняем список из исходной таблицы
    QStringList items = getItemsFromSourceTable();
    comboBox->addItems(items);
    
    return comboBox;
}

void DynamicComboBoxDelegate::setEditorData(QWidget *editor,
                                           const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
    if (!comboBox) return;

    QString currentText = index.data(Qt::EditRole).toString();
    int currentIndex = comboBox->findText(currentText);
    
    if (currentIndex >= 0) {
        comboBox->setCurrentIndex(currentIndex);
    }
}

void DynamicComboBoxDelegate::setModelData(QWidget *editor,
                                          QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
    if (!comboBox) return;

    QString selectedText = comboBox->currentText();
    model->setData(index, selectedText, Qt::EditRole);
}

void DynamicComboBoxDelegate::updateEditorGeometry(QWidget *editor,
                                                   const QStyleOptionViewItem &option,
                                                   const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

QStringList DynamicComboBoxDelegate::getItemsFromSourceTable() const
{
    QStringList items;
    if (!m_sourceTable) return items;
    
    // Исключаем последнюю строку (строку с плюсиком)
    for (int row = 0; row < m_sourceTable->rowCount() - 1; ++row) {
        QTableWidgetItem *item = m_sourceTable->item(row, m_sourceColumn);
        if (item && !item->text().isEmpty()) {
            items << item->text();
        }
    }
    
    return items;
}