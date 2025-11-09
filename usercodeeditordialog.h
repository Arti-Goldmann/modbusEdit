#ifndef USERCODEEDITORDIALOG_H
#define USERCODEEDITORDIALOG_H

#include <QDialog>
#include <QVector>
#include <QString>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include "constants.h"

// Диалог для редактирования пользовательского кода (R и/или W)
class UserCodeEditorDialog : public QDialog
{
    Q_OBJECT

public:
    // Результат редактирования
    struct Result {
        bool accepted;              // Пользователь нажал OK
        QVector<QString> code;      // Код (R для READ_ONLY, R и W для READ_WRITE)
    };

    // Конструктор
    // accessType - тип доступа ("R" или "RW")
    // initialCode - начальный код ([R] или [R, W])
    explicit UserCodeEditorDialog(const QString& accessType,
                                  const QVector<QString>& initialCode,
                                  QWidget *parent = nullptr);

    // Получить результат редактирования
    Result getResult() const;

private:
    void setupUI();
    void configTextEdit(QPlainTextEdit* textEdit, const QString& code);

    QString accessType;
    QVector<QString> initialCode;
    QVector<QPlainTextEdit*> textEditors;
    bool wasAccepted;
};

#endif // USERCODEEDITORDIALOG_H
