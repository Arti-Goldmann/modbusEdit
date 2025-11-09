#ifndef DYNAMICCOMBOBOXDELEGATE_H
#define DYNAMICCOMBOBOXDELEGATE_H

#include <QStyledItemDelegate>
#include <QComboBox>
#include <QTableWidget>

class DynamicComboBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit DynamicComboBoxDelegate(QTableWidget *sourceTable, int sourceColumn, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model,
                     const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const override;

private:
    QStringList getItemsFromSourceTable() const;

    QTableWidget *m_sourceTable;
    int m_sourceColumn;
};

#endif // DYNAMICCOMBOBOXDELEGATE_H