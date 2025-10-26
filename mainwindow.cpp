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
    ui->tableWidget->setColumnWidth(3, 100);   // Коэффициент
    ui->tableWidget->setColumnWidth(4, 80);  // Адрес (дес.)
    ui->tableWidget->setColumnWidth(5, 80);  // Адрес (hex.)
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

    // Подключаем обработку изменений в таблице данных (для синхронизации адресов)
    connect(ui->tableWidget, &QTableWidget::itemChanged,
            this, &MainWindow::onTableDataChanged);
}

void MainWindow::setupTable(QTableWidget* table) {

    //Настройка высоты строк
    table->verticalHeader()->setDefaultSectionSize(25);
    table->verticalHeader()->setFixedWidth(45);
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

    // ОТКЛЮЧАЕМ обновление таблицы, чтобы не загружать UI
    table->setUpdatesEnabled(false);
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

        // Каждые 100 строк обновляем UI и прогресс
        if (rowCounter % 100 == 0) {
            QApplication::processEvents();
        }
    }

    addPlusRow(table); //Добавляем последнюю строку с плюсиком

    // ВКЛЮЧАЕМ обновление обратно
    table->setUpdatesEnabled(true);
}

bool MainWindow::loadProfile() {

    //Выбор файла
    QString lastDir = jsonProfileManager.getLastDirectory();
    QString profilePath = QFileDialog::getOpenFileName(this,
                                                       tr("Open File"),
                                                       lastDir,
                                                       tr("JSON (*.json)"));

    if (profilePath.isEmpty()) {
        // Пользователь отменил выбор
        return false;
    }

    jsonProfileManager.setProfilePath(profilePath);

    // Создаем диалог прогресса
    progressDialog = new QProgressDialog("Загрузка профиля...", "Отмена", 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0); // Показать сразу
    progressDialog->setCancelButton(nullptr); // Убираем кнопку отмены (или оставляем для возможности отмены)
    progressDialog->show();

    // Создаем watcher для отслеживания завершения
    profileWatcher = new QFutureWatcher<std::optional<JsonProfileManager::TProfileResult>>(this);

    // Подключаем сигнал завершения
    connect(profileWatcher, &QFutureWatcher<std::optional<JsonProfileManager::TProfileResult>>::finished,
            this, &MainWindow::onProfileLoadFinished);

    // Запускаем загрузку в фоновом потоке
    QFuture<std::optional<JsonProfileManager::TProfileResult>> future = QtConcurrent::run(
        loadProfileInBackground,
        &jsonProfileManager
        );

    profileWatcher->setFuture(future);

    return true; // Вернём результат позже в onProfileLoadFinished
}

// Статическая функция, которая будет выполняться в фоновом потоке
std::optional<JsonProfileManager::TProfileResult> MainWindow::loadProfileInBackground(JsonProfileManager* manager) {
    // Эта функция выполняется в отдельном потоке!
    // Здесь НЕ ДОЛЖНО быть работы с UI!
    return manager->readProfile();
}

bool MainWindow::onProfileLoadFinished() {

    // Получаем результат из фонового потока
    auto profileResultOpt = profileWatcher->result();

    if (!profileResultOpt.has_value()) {
        // Закрываем диалог при ошибке
        progressDialog->close();
        progressDialog->deleteLater();
        profileWatcher->deleteLater();

        processError(jsonProfileManager.getLastError(), "Ошибка загрузки профиля");
        return false;
    }

    // Обновляем текст диалога перед заполнением таблиц
    progressDialog->setLabelText("Заполнение таблицы...");

    QJsonArray data = profileResultOpt.value().data;
    QJsonArray baseValues = profileResultOpt.value().baseValues;

    fillTable(data, jsonProfileManager.DATA_MAIN_KEYS, ui->tableWidget, true);
    fillTable(baseValues, jsonProfileManager.BASE_VALUES_KEYS, ui->tableWidgetBaseValues);

    QString profilePath = jsonProfileManager.getCurrentProfilePath();
    QFileInfo fileInfo(profilePath);
    ui->fileLabel->setText(QString("Файл профиля: %1").arg(fileInfo.fileName()));
    statusBar()->showMessage("Файл успешно открыт", 5000);

    //Еще устанавливаем путь генерации MBedit.c или восстанавливаем последний
    QString path = outFileGenerator.getCurrentGenFilePath();
    if(path.isEmpty()) {
        path = outFileGenerator.restoreLastGenFilePath();
    }
    QFileInfo fileGenInfo(path);
    ui->lineEditFieGenDir->setText(fileGenInfo.absolutePath());

    // Закрываем диалог прогресса ПОСЛЕ fillTable
    progressDialog->close();
    progressDialog->deleteLater();

    // Очищаем watcher
    profileWatcher->deleteLater();

    // Сбрасываем флаг изменений после загрузки профиля
    setModified(false);

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
        setModified(false);  // Сбрасываем флаг изменений после успешного сохранения
    } else {
        processError(jsonProfileManager.getLastError(), "Ошибка сохранения файла");
        return false;
    }

    return true;
}

bool MainWindow::startGeneration(){

    // Создаем диалог прогресса
    progressDialog = new QProgressDialog("Сохранение файла...", QString(), 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0);
    progressDialog->setCancelButton(nullptr);
    progressDialog->show();

    // Принудительно обрабатываем события, чтобы диалог отрисовался
    QApplication::processEvents();

    //Нужно сначала сохранить изменения в json перед тем как генерировать
    if(!saveProfile()) {
        progressDialog->close();
        progressDialog->deleteLater();
        return false;
    }

    progressDialog->setLabelText("Чтение профиля...");
    QApplication::processEvents();

    auto resultReadJsonOpt = jsonProfileManager.readProfile();

    if (!resultReadJsonOpt.has_value()) {
        progressDialog->close();
        progressDialog->deleteLater();
        processError(jsonProfileManager.getLastError(), "Ошибка генерации файла");
        return false;
    }

    QJsonArray data = resultReadJsonOpt.value().data;
    QJsonArray baseValues = resultReadJsonOpt.value().baseValues;

    if(data.isEmpty() || baseValues.isEmpty()) {
        progressDialog->close();
        progressDialog->deleteLater();
        processError(QString("Пустой профиль: %1").arg(jsonProfileManager.getCurrentProfilePath()), "Ошибка генерации файла");
        return false;
    }

    if(!outFileGenerator.hasActiveGenFile()) {
        progressDialog->close();
        progressDialog->deleteLater();
        processError(QString("Не выбран путь генерации файла"), "Ошибка генерации файла");
        return false;
    }

    progressDialog->setLabelText("Генерация файла...");
    QApplication::processEvents();

    // Создаем watcher для отслеживания завершения
    fileGenWatcher = new QFutureWatcher<bool>(this);

    // Подключаем сигнал завершения
    connect(fileGenWatcher, &QFutureWatcher<bool>::finished,
            this, &MainWindow::onGenerationFinished);

    // Запускаем генерацию в фоновом потоке
    QFuture<bool> future = QtConcurrent::run(
        genFileInBackground,
        &outFileGenerator,
        data,
        baseValues
    );

    fileGenWatcher->setFuture(future);

    return true;
}

// Статическая функция для генерации файла в фоновом потоке
bool MainWindow::genFileInBackground(OutFileGenerator* generator, QJsonArray data, QJsonArray baseValues) {
    // Эта функция выполняется в отдельном потоке!
    // Здесь НЕ ДОЛЖНО быть работы с UI!
    return generator->generate(data, baseValues);
}

bool MainWindow::onGenerationFinished(){

    // Получаем результат из фонового потока
    bool success = fileGenWatcher->result();

    // Закрываем диалог прогресса
    progressDialog->close();
    progressDialog->deleteLater();

    // Очищаем watcher
    fileGenWatcher->deleteLater();

    if(success) {
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
        setModified();
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

                setModified();
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
        setModified();
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
        setModified();
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

    setModified();
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
    }
}

void MainWindow::onBaseValuesChanged()
{
    // Пересоздаем делегат для базовых величин с обновленными данными
    delete baseValueDelegate;
    baseValueDelegate = new DynamicComboBoxDelegate(ui->tableWidgetBaseValues, 0, this);
    ui->tableWidget->setItemDelegateForColumn(TABLE_HEADERS.indexOf("Базовая величина"), baseValueDelegate);

    // Устанавливаем флаг изменений
    setModified();
}

void MainWindow::onTableDataChanged(QTableWidgetItem* item)
{
    if (!item) return;

    int row = item->row();
    int col = item->column();

    // Игнорируем последнюю строку (строка с плюсиком)
    if (row == ui->tableWidget->rowCount() - 1) return;

    int colAddressDec = TABLE_HEADERS.indexOf("Адрес (дес.)");
    int colAddressHex = TABLE_HEADERS.indexOf("Адрес (hex.)");

    // Проверяем, что изменился столбец адреса
    if (col == colAddressDec) {
        // Изменился десятичный адрес -> пересчитываем hex
        QString decText = item->text().trimmed();

        if (decText.isEmpty()) return;

        bool ok;
        int decValue = decText.toInt(&ok);

        if (ok) {
            QString hexDigits = QString("%1").arg(decValue, 0, 16, QChar('0')).toUpper();
            QString hexText = QString("0x%1").arg(hexDigits);

            // Временно отключаем сигналы, чтобы избежать рекурсии
            ui->tableWidget->blockSignals(true);

            QTableWidgetItem* hexItem = ui->tableWidget->item(row, colAddressHex);
            if (!hexItem) {
                hexItem = new QTableWidgetItem();
                ui->tableWidget->setItem(row, colAddressHex, hexItem);
            }
            hexItem->setText(hexText);

            ui->tableWidget->blockSignals(false);
        }
    }
    else if (col == colAddressHex) {
        // Изменился hex адрес -> пересчитываем десятичный
        QString hexText = item->text().trimmed();

        if (hexText.isEmpty()) return;

        // Убираем префикс 0x если есть
        if (hexText.startsWith("0x", Qt::CaseInsensitive)) {
            hexText = hexText.mid(2);
        }

        bool ok;
        int decValue = hexText.toInt(&ok, 16);

        if (ok) {
            QString decText = QString::number(decValue);

            // Временно отключаем сигналы, чтобы избежать рекурсии
            ui->tableWidget->blockSignals(true);

            QTableWidgetItem* decItem = ui->tableWidget->item(row, colAddressDec);
            if (!decItem) {
                decItem = new QTableWidgetItem();
                ui->tableWidget->setItem(row, colAddressDec, decItem);
            }
            decItem->setText(decText);

            // Обновляем hex с правильным форматом (добавляем 0x)
            QString hexDigits = QString("%1").arg(decValue, 0, 16, QChar('0')).toUpper();
            QString formattedHex = QString("0x%1").arg(hexDigits);
            item->setText(formattedHex);

            ui->tableWidget->blockSignals(false);
        }
    }

    // Устанавливаем флаг изменений
    setModified();
}

void MainWindow::setModified(bool modified) {
    hasUnsavedChanges = modified;
}

bool MainWindow::maybeSave() {
    if (!hasUnsavedChanges) {
        return true;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("modbusEdit");
    msgBox.setText("Документ был изменен.\nХотите сохранить изменения?");
    msgBox.setIcon(QMessageBox::Warning);

    QPushButton *saveButton = msgBox.addButton("Сохранить", QMessageBox::AcceptRole);
    msgBox.addButton("Не сохранять", QMessageBox::DestructiveRole);
    QPushButton *cancelButton = msgBox.addButton("Отмена", QMessageBox::RejectRole);

    msgBox.setDefaultButton(saveButton);
    msgBox.exec();

    if (msgBox.clickedButton() == saveButton) {
        return saveProfile();
    } else if (msgBox.clickedButton() == cancelButton) {
        return false;
    }

    return true;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}
