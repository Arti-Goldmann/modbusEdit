#include "usercodeeditordialog.h"

UserCodeEditorDialog::UserCodeEditorDialog(const QString& accessType,
                                           const QVector<QString>& initialCode,
                                           QWidget *parent)
    : QDialog(parent)
    , accessType(accessType)
    , initialCode(initialCode)
    , wasAccepted(false)
{
    setupUI();
}

void UserCodeEditorDialog::setupUI() {
    setWindowTitle("Редактор кода C");
    setModal(true);
    resize(600, 400);

    // Настраиваем шрифт
    QFont font("Consolas", 10);
    if (!font.exactMatch()) {
        font.setFamily("Courier New");
    }

    // Определяем количество редакторов
    int numOfAccessVarieties;
    if (accessType == Constants::AccessType::READ_ONLY) {
        numOfAccessVarieties = 1; // Только чтение
    } else if (accessType == Constants::AccessType::READ_WRITE) {
        numOfAccessVarieties = 2; // Чтение и запись
    } else {
        numOfAccessVarieties = 1; // По умолчанию
    }

    // Создаем кнопки
    QPushButton* okButton = new QPushButton("OK", this);
    QPushButton* cancelButton = new QPushButton("Отмена", this);

    // Подключаем кнопки
    connect(okButton, &QPushButton::clicked, this, [this]() {
        wasAccepted = true;
        accept();
    });
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Компоновка
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    // Создаем редакторы кода
    for (int i = 0; i < numOfAccessVarieties; i++) {
        QString userCode = (i < initialCode.size()) ? initialCode[i] : "";

        QPlainTextEdit* textEdit = new QPlainTextEdit(this);
        configTextEdit(textEdit, userCode);
        textEditors.append(textEdit);

        QString labelMessage;
        if (i == Constants::UserCodeOffsetInTable::R) {
            labelMessage = Constants::UiText::READ_LABEL;
        } else if (i == Constants::UserCodeOffsetInTable::W) {
            labelMessage = Constants::UiText::WRITE_LABEL;
        }

        mainLayout->addWidget(new QLabel(QString("Введите ваш код %1:").arg(labelMessage)));
        mainLayout->addWidget(textEdit);
    }

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);
}

void UserCodeEditorDialog::configTextEdit(QPlainTextEdit* textEdit, const QString& code) {
    textEdit->setPlainText(code);

    QFont font("Consolas", 10);
    if (!font.exactMatch()) {
        font.setFamily("Courier New");
    }
    textEdit->setFont(font);

    // Настройки редактора
    textEdit->setTabStopDistance(40);
    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
}

UserCodeEditorDialog::Result UserCodeEditorDialog::getResult() const {
    Result result;
    result.accepted = wasAccepted;

    if (wasAccepted) {
        for (auto* editor : textEditors) {
            result.code.append(editor->toPlainText());
        }
    }

    return result;
}
