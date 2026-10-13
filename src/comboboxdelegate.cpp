#include "comboboxdelegate.h"
#include <QComboBox>
#include <QTimer>

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

    // Открываем список сразу после создания редактора, чтобы одного клика по ячейке было достаточно.
    QTimer::singleShot(0, comboBox, [comboBox]() {
        comboBox->showPopup();
    });

    return comboBox;
}

void ComboBoxDelegate::setEditorData(QWidget *editor, 
                                    const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
    if (!comboBox) return;

    QString currentText = index.data(Qt::EditRole).toString();
    int currentIndex = comboBox->findText(currentText);
    
    // Для пустой ячейки не выбираем первый пункт: если пользователь закроет
    // список, ничего не выбрав, ячейка должна остаться пустой.
    comboBox->setCurrentIndex(currentIndex);
}

void ComboBoxDelegate::setModelData(QWidget *editor, 
                                   QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
    QComboBox *comboBox = qobject_cast<QComboBox*>(editor);
    if (!comboBox) return;

    // Ничего не выбрано - оставляем ячейку как есть
    if (comboBox->currentIndex() < 0) return;

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
