#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , outFileGenerator(parent)
    , jsonProfileManager(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupUI();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI(){

    // Отключить tooltip для всего приложения
    this->setToolTipDuration(0);
    
    // Рекурсивно убрать tooltip у всех дочерних виджетов
    const QList<QWidget*> widgets = this->findChildren<QWidget*>();
    for (QWidget* widget : std::as_const(widgets)) {
        widget->setToolTip("");
    }

    // Статус бар
    statusBar()->showMessage("Выберете файл профиля");

    // Размер окна
    resize(1200, 1000);
    setWindowTitle("modbusEdit");
    setWindowIcon(QIcon(":/ModBus.ico"));

    //Настройки таблицы с данными
    ui->tableWidget->setColumnCount(TABLE_HEADERS.size());
    ui->tableWidget->setHorizontalHeaderLabels(TABLE_HEADERS);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    // Настройка ширины столбцов
    ui->tableWidget->setColumnWidth(0, 300);  // Название группы параметров
    ui->tableWidget->setColumnWidth(1, 100);  // Тип доступа
    ui->tableWidget->setColumnWidth(2, 100);  // Тип данных
    ui->tableWidget->setColumnWidth(3, 80);   // Коэффициент
    ui->tableWidget->setColumnWidth(4, 80);   // Адрес (дес.)
    ui->tableWidget->setColumnWidth(5, 80);   // Адрес (hex.)
    ui->tableWidget->setColumnWidth(6, 200);  // Переменнная
    ui->tableWidget->setColumnWidth(7, 200);  // Базовая величина
    ui->tableWidget->setColumnWidth(8, 150);  // Примечание

    setupTable(ui->tableWidget);
    addPlusRow(ui->tableWidget); //Добавляем последнюю строку с плюсиком
    
    // Настройка делегатов для комбобоксов
    QStringList accessTypes = {"RW", "R"};
    accessTypeDelegate = new ComboBoxDelegate(accessTypes, this);
    
    QStringList dataTypes = {"int16", "Uint16"};
    dataTypeDelegate = new ComboBoxDelegate(dataTypes, this);
    
    baseValueDelegate = new DynamicComboBoxDelegate(ui->tableWidgetBaseValues, 0, this);
    
    // Устанавливаем делегаты для соответствующих столбцов
    ui->tableWidget->setItemDelegateForColumn(1, accessTypeDelegate);  // Тип доступа
    ui->tableWidget->setItemDelegateForColumn(2, dataTypeDelegate);    // Тип данных
    ui->tableWidget->setItemDelegateForColumn(7, baseValueDelegate);   // Базовая величина

    //Настройки таблицы с базовыми величинами
    ui->tableWidgetBaseValues->setColumnCount(BASE_VALUES_HEADERS.size());
    ui->tableWidgetBaseValues->setHorizontalHeaderLabels(BASE_VALUES_HEADERS);
    ui->tableWidgetBaseValues->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->tableWidgetBaseValues->setColumnWidth(0, 300);
    ui->tableWidgetBaseValues->setColumnWidth(1, 70);
    ui->tableWidgetBaseValues->setColumnWidth(2, 70);
    ui->tableWidgetBaseValues->setColumnWidth(3, 200);
    ui->tableWidgetBaseValues->setColumnWidth(4, 300);

    setupTable(ui->tableWidgetBaseValues);
    addPlusRow(ui->tableWidgetBaseValues); //Добавляем последнюю строку с плюсиком
    
    // Создаем делегат для формата IQ
    QStringList iqFormats;
    for(int i = 16; i >= 0; --i) {
        iqFormats << QString("%1.%2").arg(i).arg(16 - i);
    }
    iqFormatDelegate = new ComboBoxDelegate(iqFormats, this);
    
    // Устанавливаем делегат для столбца "Формат IQ"
    ui->tableWidgetBaseValues->setItemDelegateForColumn(2, iqFormatDelegate);

    connect(ui->toolBtnGenPath, &QPushButton::clicked,
            this, &MainWindow::setGenerationPath);

    connect(ui->startGenBtn, &QPushButton::clicked,
            this, &MainWindow::startGeneration);

    connect(ui->open, &QAction::triggered,
            this, &MainWindow::loadProfile);

    connect(ui->save, &QAction::triggered,
            this, &MainWindow::saveProfile);

    connect(ui->saveAs, &QAction::triggered,
            this, &MainWindow::saveProfileAs);

    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);

    connect(ui->tableWidget, &QTableWidget::cellClicked,
            this, &MainWindow::onCellClickedData);

    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked,
            this, &MainWindow::onCellDoubleClickedData);

    connect(ui->tableWidgetBaseValues, &QTableWidget::cellClicked,
            this, &MainWindow::onCellClickedBaseValues);

    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    connect(ui->tableWidgetBaseValues, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenuBaseValues);

    // Подключаем автообновление базовых величин
    connect(ui->tableWidgetBaseValues, &QTableWidget::itemChanged,
            this, &MainWindow::onBaseValuesChanged);
}

void MainWindow::setupTable(QTableWidget* table) {

    //Настройка высоты строк
    table->verticalHeader()->setDefaultSectionSize(30);
    table->verticalHeader()->setFixedWidth(40);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed); //Нельзя менять высоту строки
    table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter); //Выравнивание по центру
    table->setSelectionBehavior(QAbstractItemView::SelectItems); //Поведение при выделении ячейки
    table->setSelectionMode(QAbstractItemView::SingleSelection); //Только одна ячейка
    table->setAlternatingRowColors(true); //Делает строку черно-белой через одну
    table->horizontalHeader()->setStretchLastSection(true); //Растягивает последний столбец на всю оставшуюся ширину таблицы

    //Настройка стиля
    table->setStyleSheet(
        "QHeaderView::section {"
        "    border: 1px solid black;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: transparent;"
        "    selection-background-color: transparent;"
        "    border: 2px solid black;"
        "}"
        );
}

bool MainWindow::setGenerationPath() {

    if(outFileGenerator.setGenerationPath()) {
        QString path = outFileGenerator.getCurrentGenFilePath();
        QFileInfo fileInfo(path);
        ui->absPathToFileGenLabel->setText(QString("Путь генерации файла: %1").arg(fileInfo.absoluteFilePath()));
        ui->lineEditFieGenDir->setText(fileInfo.absolutePath());
    } else {
        processError(outFileGenerator.getLastError(), "Ошибка генерации файла");
        return false;
    }

    return true;
}

void MainWindow::fillTable(const QJsonArray& data, const QStringList& mainKeys, QTableWidget* table, bool configRow) {
    // Заполняем таблицу с данными
    table->setRowCount(0);

    int rowCounter = 0;
    for(const QJsonValue &val : std::as_const(data)) {

        QJsonObject obj = val.toObject();

        // Добавляем строку в таблицу
        table->insertRow(rowCounter);

        //Получаем строку из объекта json и кладем в соответсвующую колонку
        for(int col = 0; col < mainKeys.size(); col++) {
            QString key = mainKeys[col];
            QString str = obj[key].toString();
            table->setItem(rowCounter, col, new QTableWidgetItem(str));
        }

        if(configRow) { //Конфигурируем строки
            QString paramType = obj["paramType"].toString();

            if(paramType == "commonType") { //Встяавляем в соответсвующую колонку название перемеенной
                QString varName = obj["varName"].toString();
                QVector<QString> data = {varName};
                setRowType(paramType, rowCounter, table, data);
            } else if(paramType == "userType") {
                //Пользовательская ячейка. Выставляем PASTE YOUR CODE в ячейке, если пустая: USER CODE
                QString userCode_R = obj["userCode_R"].toString();
                QString userCode_W = obj["userCode_W"].toString();
                QVector<QString> data = {userCode_R, userCode_W};
                QString accessType = obj["accessType"].toString();
                setRowType(paramType, rowCounter, table, data, accessType);
            }
        }
        rowCounter++;
    }

    addPlusRow(table); //Добавляем последнюю строку с плюсиком
}

bool MainWindow::loadProfile() {

    auto profileResultOpt = jsonProfileManager.loadProfile();

    if (!profileResultOpt.has_value()) {
        processError(jsonProfileManager.getLastError(), "Ошибка загрузки профиля");
        return false;
    }

    QJsonArray data = profileResultOpt.value().data;
    QJsonArray baseValues = profileResultOpt.value().baseValues;

    fillTable(data, jsonProfileManager.DATA_MAIN_KEYS, ui->tableWidget, true);
    fillTable(baseValues, jsonProfileManager.BASE_VALUES_KEYS, ui->tableWidgetBaseValues);

    QString profilePath = jsonProfileManager.getCurrentProfilePath();
    QFileInfo fileInfo(profilePath);
    ui->fileLabel->setText(QString("Файл профиля: %1").arg(fileInfo.fileName()));
    statusBar()->showMessage("Файл успешно открыт", 5000);
    return true;
}

bool MainWindow::saveProfile() {
    bool isSaveAs = false;
    return saveProfileHandler(isSaveAs);
}

bool MainWindow::saveProfileAs() {
    bool isSaveAs = true;
    return saveProfileHandler(isSaveAs);
}

bool MainWindow::saveProfileHandler(bool isSaveAs) {

    bool result = isSaveAs ? jsonProfileManager.saveProfileAs(ui->tableWidget, ui->tableWidgetBaseValues) :
                             jsonProfileManager.saveProfile(ui->tableWidget, ui->tableWidgetBaseValues);

    if(result) {
        QString profilePath = jsonProfileManager.getCurrentProfilePath();
        QFileInfo fileInfo(profilePath);
        ui->fileLabel->setText(QString("Файл профиля: %1").arg(fileInfo.fileName()));
        statusBar()->showMessage("Файл успешно сохранен", 5000);
    } else {
        processError(jsonProfileManager.getLastError(), "Ошибка сохранения файла");
        return false;
    }

    return true;
}

bool MainWindow::startGeneration(){

    //TODO: нужно сначал сохранить изменения в json перед тем как генерировать, если были изменения в таблице

    auto resultReadJsonOpt = jsonProfileManager.readProfile();

    if (!resultReadJsonOpt.has_value()) {
        processError(jsonProfileManager.getLastError(), "Ошибка генерации файла");
        return false;
    }

    QJsonArray data = resultReadJsonOpt.value().data;
    QJsonArray baseValues = resultReadJsonOpt.value().baseValues;

    if(data.isEmpty() || baseValues.isEmpty()) {
        processError(QString("Пустой профиль: %1").arg(jsonProfileManager.getCurrentProfilePath()), "Ошибка генерации файла");
        return false;
    }

    if(!outFileGenerator.hasActiveGenFile()) {
        processError(QString("Не выбран путь генерации файла"), "Ошибка генерации файла");
        return false;
    }

    if(outFileGenerator.generate(data, baseValues)) {
        statusBar()->showMessage("Файл успешно сгенерирован", 5000);
        
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setWindowIcon(QIcon(":/ModBus.ico"));
        msgBox.setWindowTitle("Генерация завершена");
        msgBox.setText("Файл успешно сгенерирован!");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    } else {
        processError(outFileGenerator.getLastError(), "Ошибка генерации файла");
        return false;
    }

    return true;
}

void MainWindow::addPlusRow(QTableWidget* table) {
    int row = table->rowCount();
    table->insertRow(row);

    // Объединить все столбцы в одну ячейку
    table->setSpan(row, 0, 1, table->columnCount()); // row, column, rowSpan, columnSpan

    // Создать элемент с плюсиком
    QTableWidgetItem *item = new QTableWidgetItem(" +");

    QFont font;
    font.setPointSize(18);        // размер шрифта
    font.setBold(true);           // жирный

    item->setFont(font);
    item->setForeground(Qt::green);
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Сделать нередактируемой
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);

    table->setItem(row, 0, item);
}

void MainWindow::onSelectionChanged(){

}

// Реализация слота для клика по ячейке
void MainWindow::onCellClickedData(int row, int col)
{
    if (row == ui->tableWidget->rowCount() - 1) {
        // Клик по последней строке - вставляем новую ПЕРЕД ней
        ui->tableWidget->insertRow(row);
    }
}

static void configTextEdit(QPlainTextEdit* textEdit, const QString& code, const QFont& font) {
    textEdit->setPlainText(code);
    textEdit->setFont(font);

    // Настройки редактора
    textEdit->setTabStopDistance(40);
    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
}

// Реализация слота для дабл клика по ячейке
void MainWindow::onCellDoubleClickedData(int row, int col)
{
    //Не последняя строка
    if (row != ui->tableWidget->rowCount() - 1) {

        // Проверяем, что это столбец "Переменная/значение" и тип строки "userType"
        if (col != TABLE_HEADERS.indexOf("Переменная / значение")) return;

        QTableWidgetItem* rootItem = ui->tableWidget->item(row, 0); //Скрытая информация текущей строки в 0 ячейке
        QString rowType = rootItem ? rootItem->data(Qt::UserRole).toString() : "";

        if (rowType == "userType") {

            QTableWidgetItem* accessTypeItem = ui->tableWidget->item(row,  TABLE_HEADERS.indexOf("Тип доступа"));
            QString accessType = accessTypeItem ? accessTypeItem->text() : ""; //RW или R

            int numOfAccessVarieties;

            if(accessType == "R") {
                //Если тип доступа R, то нужно редактировать код только на чтение (R)
                numOfAccessVarieties = 1;
            } else if(accessType == "RW") {
                //Если тип доступа RW, то нужно редактировать код и на чтение (R) и на запись (W)
                numOfAccessVarieties = 2;
            } else {
                processError("Неизвестный тип доступа у параметра", "Ошибка редактирования пользовательского кода");
                return;
            }

            // Создаем диалог
            QDialog* dialog = new QDialog(this);
            dialog->setWindowTitle("Редактор кода C");
            dialog->setModal(true);
            dialog->resize(600, 400);

            // Настраиваем шрифт
            QFont font("Consolas", 10);
            if (!font.exactMatch()) {
                font.setFamily("Courier New");
            }

            // Кнопки
            QPushButton* okButton = new QPushButton("OK", dialog);
            QPushButton* cancelButton = new QPushButton("Отмена", dialog);

            // Подключаем кнопки
            connect(okButton, &QPushButton::clicked, dialog, &QDialog::accept);
            connect(cancelButton, &QPushButton::clicked, dialog, &QDialog::reject);

            // Компоновка
            QVBoxLayout* mainLayout = new QVBoxLayout(dialog);
            QHBoxLayout* buttonLayout = new QHBoxLayout();

            QString userCodeArr[numOfAccessVarieties];
            QPlainTextEdit* textEditArr[numOfAccessVarieties];

            for(int i = 0; i < numOfAccessVarieties; i++) {
                // Получаем текущий пользовательский код на чтение или запись
                QVariant data =  rootItem->data(Qt::UserRole + 1 + i);
                QString userCode = data.isValid() ? data.toString() : "";
                userCodeArr[i] = userCode;

                // Создаем QPlainTextEdit
                textEditArr[i] = new QPlainTextEdit(dialog);
                configTextEdit(textEditArr[i], userCodeArr[i], font);

                QString labelMessage;
                if(i == R) {
                    labelMessage = "на чтение (R)";
                } else if(i == W) {
                    labelMessage = "на запись (W)";
                }
                mainLayout->addWidget(new QLabel(QString("Введите ваш код %1:").arg(labelMessage)));
                mainLayout->addWidget(textEditArr[i]);
            }

            buttonLayout->addStretch();
            buttonLayout->addWidget(okButton);
            buttonLayout->addWidget(cancelButton);
            mainLayout->addLayout(buttonLayout);

            // Показываем диалог
            if (dialog->exec() == QDialog::Accepted) {

                bool isCodeEmpty = true;
                QString newCodeArr[numOfAccessVarieties];
                for(int i = 0; i < numOfAccessVarieties; i++) {
                    newCodeArr[i] = textEditArr[i]->toPlainText();
                    // Сохраняем код
                    rootItem->setData(Qt::UserRole + 1 + i, newCodeArr[i]);
                    if(!newCodeArr[i].isEmpty()) {
                        isCodeEmpty = false;
                    }
                }

                // Обновляем отображение в ячейке
                QTableWidgetItem* item = ui->tableWidget->item(row, col);

                if (isCodeEmpty) {
                    if(item) {
                        item->setText("*** PASTE YOUR CODE ***");
                        item->setForeground(QBrush());  // Пустая кисть = дефолтный цвет
                    }
                } else {
                    if(item) {
                        item->setText("*** USER CODE ✓ ***");
                        item->setForeground(QBrush(Qt::green));
                    }
                }
            }

            dialog->deleteLater();
        }
    }
}
void MainWindow::onCellClickedBaseValues(int row, int col)
{
    if (row == ui->tableWidgetBaseValues->rowCount() - 1) {
        // Клик по последней строке - вставляем новую ПЕРЕД ней
        ui->tableWidgetBaseValues->insertRow(row);
    }
}

void MainWindow::processError(const QString& message, const QString& title) {
    statusBar()->showMessage(title, 5000);
    qCritical() << message;
    QMessageBox::critical(this, title, message);
}

void MainWindow::showContextMenu(const QPoint &pos)
{
    showContextMenuForTable(pos, ui->tableWidget);
}

void MainWindow::showContextMenuBaseValues(const QPoint &pos)
{
    showContextMenuForTable(pos, ui->tableWidgetBaseValues);
}

void MainWindow::showContextMenuForTable(const QPoint &pos, QTableWidget* table)
{
    int row = table->rowAt(pos.y());
    if (row < 0) return;
    
    // Не показывать контекстное меню для последней строки (строки с плюсиком)
    if (row == table->rowCount() - 1) {
        return;
    }
    
    // Сохраняем номер строки и таблицу для использования в других методах
    contextMenuClickRow = row;
    contextMenuActiveTable = table;
    
    QMenu contextMenu(this);

    QAction *deleteAction = contextMenu.addAction("Удалить");
    QAction *addAction = contextMenu.addAction("Добавить");

    QAction *commonType = nullptr;
    QAction *userType = nullptr;
    if(contextMenuActiveTable == ui->tableWidget) {// Если основная таблица с данными
        QMenu *subMenu = contextMenu.addMenu("Тип параметра");
        commonType = subMenu->addAction("Обычный");
        userType = subMenu->addAction("Пользовательский");
    }


    QAction *selectedAction = contextMenu.exec(table->mapToGlobal(pos));

    if (selectedAction == deleteAction) {
        deleteRow();
    } else if (selectedAction == addAction) {
        addRow();
    } else if (commonType && selectedAction == commonType) {
        setRowType("commonType", contextMenuClickRow, contextMenuActiveTable);
    } else if(userType && selectedAction == userType) {
        setRowType("userType", contextMenuClickRow, contextMenuActiveTable);
    }
}

void MainWindow::deleteRow()
{
    if (!contextMenuActiveTable) return;
    
    if (contextMenuClickRow >= 0 && contextMenuClickRow < contextMenuActiveTable->rowCount() - 1) {
        contextMenuActiveTable->removeRow(contextMenuClickRow);
    }
}

void MainWindow::addRow()
{
    if (!contextMenuActiveTable) return;

    int rowIndex = -1;
    
    if (contextMenuClickRow >= 0) {
        // Добавляем строку после той, на которую кликнули
        rowIndex = contextMenuClickRow + 1;
        contextMenuActiveTable->insertRow(rowIndex);

    } else {
        // Если не знаем где кликнули, добавляем перед последней строкой (строкой с плюсиком)
        rowIndex = contextMenuActiveTable->rowCount() - 1;
        contextMenuActiveTable->insertRow(rowIndex);
    }

    //Если основная таблица с данными, то определим дефолтный тип строки
    if(contextMenuActiveTable == ui->tableWidget && rowIndex > -1) {
        setRowType("commonType", rowIndex, contextMenuActiveTable);
    }
}

void MainWindow::setRowType(const QString& rowType, int rowIndex, QTableWidget* table, const QVector<QString>& data, const QString& accessType) {

    if (!table) return;

    //Сохраняем инфу с типом параметра в 0 колонке
    QTableWidgetItem *itemRoot = table->item(rowIndex, 0);
    if (!itemRoot) {
        itemRoot = new QTableWidgetItem();
        table->setItem(rowIndex, 0, itemRoot);
    }
    itemRoot->setData(Qt::UserRole, rowType);

    int columnIndex = TABLE_HEADERS.indexOf("Переменная / значение");
    if (columnIndex == -1) return;

    //В колонку "Переменная / значение" устаналиваем текст и настройки в зависимости от типа
    QTableWidgetItem *item = table->item(rowIndex, columnIndex);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(rowIndex, columnIndex, item);
    }

    if(rowType == "commonType") {
        QString varName = !data.isEmpty() ? data[0] : "";
        item->setText(varName); //Переменная хранится в 0 элементе
        item->setFlags(item->flags() | Qt::ItemIsEditable); // делаем редактируемым

        qDebug() << "Записали для строки " << rowIndex << "переменную: " << data;

    } else if (rowType == "userType") {
        bool isEmpty = true;
        //В корневой элемент сохраняем пользовательский код
        if(accessType == "R") { //Если R, то тогда нам должны были передать только посльзовательский код на чтение
            QString userCode_R = !data.isEmpty() ? data[R] : "";
            if(userCode_R != "") isEmpty = false;
            itemRoot->setData(Qt::UserRole + 1 + R, userCode_R);

        } else if(accessType == "RW") { //Если RW, то тогда должен быть пользовательский код и на чтение и на запись
            QString userCode_R = !data.isEmpty() ? data[R] : "";
            QString userCode_W = !data.isEmpty() ? data[W] : "";

            if(userCode_R != "" || userCode_W != "") isEmpty = false;

            itemRoot->setData(Qt::UserRole + 1 + R, userCode_R);
            itemRoot->setData(Qt::UserRole + 1 + W, userCode_W);
        }

        if(isEmpty) {
            item->setText("*** PASTE YOUR CODE ***");
            item->setForeground(QBrush());  // Пустая кисть = дефолтный цвет
        } else {
            item->setText("*** USER CODE ✓ ***");
            item->setForeground(QBrush(Qt::green));
        }
        item->setFlags(item->flags() & ~Qt::ItemIsEditable); // убираем возможность редактирования

        qDebug() << "Сохранили для строки " << rowIndex << "код: " << data;
    }
}

void MainWindow::onBaseValuesChanged()
{
    // Пересоздаем делегат для базовых величин с обновленными данными
    delete baseValueDelegate;
    baseValueDelegate = new DynamicComboBoxDelegate(ui->tableWidgetBaseValues, 0, this);
    ui->tableWidget->setItemDelegateForColumn(TABLE_HEADERS.indexOf("Базовая величина"), baseValueDelegate);
}
