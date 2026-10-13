#ifndef NOTEDELEGATE_H
#define NOTEDELEGATE_H

#include <QStyledItemDelegate>

// Делегат для столбца "Примечание".
// В таблице примечание показывается одной строкой (переносы заменены на " ↵ ",
// длинный текст обрезается "..."), высота строки не меняется.
// При редактировании поверх таблицы раскрывается многострочное поле шириной столбца,
// которое не влияет на размеры строк и после завершения снова сворачивается.
class NoteDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit NoteDelegate(QObject* parent = nullptr);

    QString displayText(const QVariant& value, const QLocale& locale) const override;

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;

protected:
    void initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const override;
    bool eventFilter(QObject* object, QEvent* event) override;
};

#endif // NOTEDELEGATE_H
