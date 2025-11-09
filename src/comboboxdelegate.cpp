#include "comboboxdelegate.h"
#include <QComboBox>

ComboBoxDelegate::ComboBoxDelegate(const QStringList &items, QObject *parent)
    : QStyledItemDelegate(parent), m_items(items)
{
}

QWidget *ComboBoxDelegate::createEditor(QWidget *parent, 
                                       const QStyleOptionViewItem &option,
                                       const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->addItems(m_items);
    return comboBox;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, 
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

void ComboBoxDelegate::setModelData(QWidget *editor, 
                                   QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
    if (!comboBox) return;

    QString selectedText = comboBox->currentText();
    model->setData(index, selectedText, Qt::EditRole);
}

void ComboBoxDelegate::updateEditorGeometry(QWidget *editor,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}