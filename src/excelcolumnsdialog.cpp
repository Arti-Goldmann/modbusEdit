#include "excelcolumnsdialog.h"

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSettings>
#include <QVBoxLayout>

ExcelColumnsDialog::ExcelColumnsDialog(Mode mode, const QStringList& groups, QWidget* parent)
    : QDialog(parent), mode(mode)
{
    const bool isExport = (mode == Mode::Export);
    setWindowTitle(isExport ? "Экспорт в Excel" : "Импорт из Excel");

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel(isExport ? "Выберите столбцы и группы параметров для экспорта:"
                                              : "Выберите столбцы и группы параметров для импорта:", this));

    // Слева - столбцы листов, справа - группы параметров
    QHBoxLayout* contentLayout = new QHBoxLayout();
    QVBoxLayout* columnsLayout = new QVBoxLayout();
    columnsLayout->addWidget(createGroup(QString("Лист «%1»").arg(ExcelExchange::dataSheetName()),
                                         ExcelExchange::dataColumns(), "data", dataBoxes));
    columnsLayout->addWidget(createGroup(QString("Лист «%1»").arg(ExcelExchange::baseSheetName()),
                                         ExcelExchange::baseColumns(), "base", baseBoxes));
    columnsLayout->addStretch();
    contentLayout->addLayout(columnsLayout);
    contentLayout->addWidget(createGroupsBox(groups), 1);
    mainLayout->addLayout(contentLayout);

    if (!isExport) {
        QLabel* hint = new QLabel("Таблицы будут заменены содержимым файла. Если на листе не отмечено "
                                  "ни одного столбца, соответствующая таблица останется без изменений.", this);
        hint->setWordWrap(true);
        mainLayout->addWidget(hint);
    }

    QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
    okButton = buttonBox->addButton(isExport ? "Экспорт..." : "Импорт...", QDialogButtonBox::AcceptRole);
    buttonBox->addButton("Отмена", QDialogButtonBox::RejectRole);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ExcelColumnsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    updateOkButton();
}

QString ExcelColumnsDialog::settingsPrefix() const {
    return mode == Mode::Export ? "excelColumns/export/" : "excelColumns/import/";
}

QWidget* ExcelColumnsDialog::createGroup(const QString& title, const QVector<ExcelExchange::Column>& columns,
                                         const QString& settingsKey, QVector<Box>& boxes) {
    QGroupBox* group = new QGroupBox(title, this);
    QVBoxLayout* groupLayout = new QVBoxLayout(group);

    // Восстанавливаем сохраненный выбор, при первом запуске - значения по умолчанию
    QSettings settings("MPEI", "modbusEdit");
    const QString key = settingsPrefix() + settingsKey;
    const bool hasSaved = settings.contains(key);
    const QStringList saved = settings.value(key).toStringList();

    QGridLayout* grid = new QGridLayout();
    const int rowsPerColumn = (columns.size() + 1) / 2; // Две колонки галочек
    for (int i = 0; i < columns.size(); i++) {
        QCheckBox* checkBox = new QCheckBox(columns[i].header, group);
        checkBox->setChecked(hasSaved ? saved.contains(columns[i].id) : columns[i].defaultOn);
        connect(checkBox, &QCheckBox::toggled, this, &ExcelColumnsDialog::updateOkButton);
        grid->addWidget(checkBox, i % rowsPerColumn, i / rowsPerColumn);
        boxes.append({columns[i].id, checkBox});
    }
    groupLayout->addLayout(grid);

    return group;
}

QWidget* ExcelColumnsDialog::createGroupsBox(const QStringList& groups) {
    QGroupBox* box = new QGroupBox("Группы параметров (заголовки)", this);
    QVBoxLayout* boxLayout = new QVBoxLayout(box);

    // Группы у каждого профиля свои, поэтому запоминаем снятые галочки:
    // новые группы по умолчанию отмечены
    QSettings settings("MPEI", "modbusEdit");
    const QStringList unchecked = settings.value(settingsPrefix() + "uncheckedGroups").toStringList();

    QWidget* listWidget = new QWidget();
    QVBoxLayout* listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(4, 4, 4, 4);
    for (const QString& group : groups) {
        QCheckBox* checkBox = new QCheckBox(group, listWidget);
        checkBox->setChecked(!unchecked.contains(group));
        connect(checkBox, &QCheckBox::toggled, this, &ExcelColumnsDialog::updateOkButton);
        listLayout->addWidget(checkBox);
        groupBoxes.append({group, checkBox});
    }
    if (groups.isEmpty()) {
        listLayout->addWidget(new QLabel("Групп нет", listWidget));
    }
    listLayout->addStretch();

    // Групп может быть много - список прокручивается
    QScrollArea* scroll = new QScrollArea(box);
    scroll->setWidget(listWidget);
    scroll->setWidgetResizable(true);
    scroll->setMinimumWidth(320);
    boxLayout->addWidget(scroll);

    QHBoxLayout* buttons = new QHBoxLayout();
    QPushButton* selectAll = new QPushButton("Выбрать все", box);
    QPushButton* clearAll = new QPushButton("Снять все", box);
    buttons->addWidget(selectAll);
    buttons->addWidget(clearAll);
    buttons->addStretch();
    boxLayout->addLayout(buttons);

    connect(selectAll, &QPushButton::clicked, this, [this]() { setAll(groupBoxes, true); });
    connect(clearAll, &QPushButton::clicked, this, [this]() { setAll(groupBoxes, false); });

    return box;
}

QStringList ExcelColumnsDialog::selectedIds(const QVector<Box>& boxes) {
    QStringList ids;
    for (const Box& box : boxes) {
        if (box.checkBox->isChecked()) ids << box.id;
    }
    return ids;
}

void ExcelColumnsDialog::setAll(const QVector<Box>& boxes, bool checked) {
    for (const Box& box : boxes) {
        box.checkBox->setChecked(checked);
    }
}

void ExcelColumnsDialog::updateOkButton() {
    if (okButton) {
        // Для листа карты регистров нужны и столбцы, и хотя бы одна группа
        const bool dataOk = !selectedDataColumns().isEmpty() &&
                            (groupBoxes.isEmpty() || !selectedGroups().isEmpty());
        okButton->setEnabled(dataOk || !selectedBaseColumns().isEmpty());
    }
}

void ExcelColumnsDialog::accept() {
    // Запоминаем выбор, чтобы в следующий раз не отмечать галочки заново
    QSettings settings("MPEI", "modbusEdit");
    settings.setValue(settingsPrefix() + "data", selectedDataColumns());
    settings.setValue(settingsPrefix() + "base", selectedBaseColumns());

    // Снятые группы добавляем к ранее снятым, отмеченные - убираем из списка
    QStringList unchecked = settings.value(settingsPrefix() + "uncheckedGroups").toStringList();
    for (const Box& box : std::as_const(groupBoxes)) {
        unchecked.removeAll(box.id);
        if (!box.checkBox->isChecked()) unchecked << box.id;
    }
    settings.setValue(settingsPrefix() + "uncheckedGroups", unchecked);
    QDialog::accept();
}
