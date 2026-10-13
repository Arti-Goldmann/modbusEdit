#include "notedelegate.h"

#include <QKeyEvent>
#include <QPlainTextEdit>

namespace {
constexpr int EDITOR_HEIGHT = 180;     // Высота раскрытого поля
}

NoteDelegate::NoteDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QString NoteDelegate::displayText(const QVariant& value, const QLocale& locale) const {
    // В свернутом виде - одна строка: переносы показываем значком
    QString text = QStyledItemDelegate::displayText(value, locale);
    text.replace("\r\n", "\n");
    text.replace('\n', QString::fromUtf8(" ↵ "));
    return text;
}

void NoteDelegate::initStyleOption(QStyleOptionViewItem* option, const QModelIndex& index) const {
    QStyledItemDelegate::initStyleOption(option, index);
    // Без переноса слов длинный текст обрезается "..." и строка не растет
    option->features &= ~QStyleOptionViewItem::WrapText;
    option->textElideMode = Qt::ElideRight;
}

QWidget* NoteDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
    Q_UNUSED(option)
    Q_UNUSED(index)

    QPlainTextEdit* editor = new QPlainTextEdit(parent);
    editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    editor->setTabChangesFocus(true);
    editor->setFrameShape(QFrame::Box);
    editor->setStyleSheet("QPlainTextEdit { border: 2px solid black; }");
    return editor;
}

void NoteDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    QPlainTextEdit* textEdit = qobject_cast<QPlainTextEdit*>(editor);
    if (!textEdit) return;

    textEdit->setPlainText(index.data(Qt::EditRole).toString());
    textEdit->moveCursor(QTextCursor::End);
}

void NoteDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                const QModelIndex& index) const {
    QPlainTextEdit* textEdit = qobject_cast<QPlainTextEdit*>(editor);
    if (!textEdit) return;

    const QString text = textEdit->toPlainText();
    if (text != index.data(Qt::EditRole).toString()) {
        model->setData(index, text, Qt::EditRole);
    }
}

void NoteDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const {
    Q_UNUSED(index)

    // Поле раскрывается поверх соседних ячеек и строк (редактор - дочерний
    // виджет viewport), поэтому высота строк таблицы не меняется
    // Ширина - как у столбца "Примечание", раскрывается только по высоте
    QRect rect = option.rect;
    rect.setHeight(qMax(rect.height(), EDITOR_HEIGHT));

    // Не даем полю вылезти за видимую область таблицы
    const QWidget* viewport = editor->parentWidget();
    if (viewport) {
        const QRect area = viewport->rect();
        if (rect.bottom() > area.bottom()) {
            // Внизу места нет - раскрываем вверх от ячейки
            rect.moveBottom(option.rect.bottom());
        }
        if (rect.top() < area.top()) rect.moveTop(area.top());
    }

    editor->setGeometry(rect);
    editor->raise();
}

bool NoteDelegate::eventFilter(QObject* object, QEvent* event) {
    // Enter - перенос строки (так работает QPlainTextEdit),
    // Ctrl+Enter - сохранить и свернуть
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        const bool isEnter = keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter;
        if (isEnter && (keyEvent->modifiers() & Qt::ControlModifier)) {
            QWidget* editor = qobject_cast<QWidget*>(object);
            if (editor) {
                emit commitData(editor);
                emit closeEditor(editor, QAbstractItemDelegate::NoHint);
                return true;
            }
        }
    }
    return QStyledItemDelegate::eventFilter(object, event);
}
