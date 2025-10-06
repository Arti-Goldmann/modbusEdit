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

    ui->tableWidget->setColumnCount(TABLE_HEADERS.size());
    ui->tableWidget->setHorizontalHeaderLabels(TABLE_HEADERS);

    // Настройка ширины столбцов
    ui->tableWidget->setColumnWidth(0, 300);  // Название группы параметров
    ui->tableWidget->setColumnWidth(1, 100);  // Тип доступа
    ui->tableWidget->setColumnWidth(2, 100);  // Тип данных
    ui->tableWidget->setColumnWidth(3, 80);   // Коэффициент
    ui->tableWidget->setColumnWidth(4, 60);   // Ед. изм.
    ui->tableWidget->setColumnWidth(5, 120);  // Диапазон значений
    ui->tableWidget->setColumnWidth(6, 80);   // Адрес (дес.)
    ui->tableWidget->setColumnWidth(7, 80);   // Адрес (hex.)
    ui->tableWidget->setColumnWidth(8, 150);  // Примечание

    //Настройка высоты строк
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(30);
    ui->tableWidget->verticalHeader()->setFixedWidth(40);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed); //Нельзя менять высоту строки
    ui->tableWidget->verticalHeader()->setDefaultAlignment(Qt::AlignCenter); //Выравнивание по центру
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems); //Поведение при выделении ячейки
    ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection); //Только одна ячейка
    ui->tableWidget->setAlternatingRowColors(true); //Делает строку черно-белой через одну
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true); //Растягивает последний столбец на всю оставшуюся ширину таблицы

    addPlusRow(); //Добавляем последнюю строку с плюсиком

    //Настройка стиля
    ui->tableWidget->setStyleSheet(
        "QHeaderView::section {"
        "    border: 1px solid black;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: transparent;"
        "    selection-background-color: transparent;"
        "    border: 2px solid black;"
        "}"
        );

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
            this, &MainWindow::onCellClicked);
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

bool MainWindow::loadProfile() {

    auto optionalDataJsonArr = jsonProfileManager.loadProfile();

    if (!optionalDataJsonArr.has_value()) {
        processError(jsonProfileManager.getLastError(), "Ошибка загрузки профиля");
        return false;
    }

    QJsonArray jsonArr = optionalDataJsonArr.value();

    // Очищаем таблицу
    ui->tableWidget->setRowCount(0);

    int rowCounter = 0;
    for(const QJsonValue &val : std::as_const(jsonArr)) {

        QJsonObject obj = val.toObject();

        // Добавляем строку в таблицу
        ui->tableWidget->insertRow(rowCounter);

        //Получаем строку из объекта json и кладем в соответсвующую колонку
        for(int col = 0; col < jsonProfileManager.COLUMN_KEYS.size(); col++) {
            QString str = obj[jsonProfileManager.COLUMN_KEYS[col]].toString();
            ui->tableWidget->setItem(rowCounter, col, new QTableWidgetItem(str));
        }
        rowCounter++;
    }

    addPlusRow(); //Добавляем последнюю строку с плюсиком

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

    bool result = isSaveAs ? jsonProfileManager.saveProfileAs(ui->tableWidget) :
                             jsonProfileManager.saveProfile(ui->tableWidget);

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

    auto optionalDataJsonArr = jsonProfileManager.readJsonFile();

    if (!optionalDataJsonArr.has_value()) {
        processError(jsonProfileManager.getLastError(), "Ошибка генерации файла");
        return false;
    }

    QJsonArray jsonArr = optionalDataJsonArr.value();

    if(jsonArr.isEmpty()) {
        processError(QString("Пустой профиль: %1").arg(jsonProfileManager.getCurrentProfilePath()), "Ошибка генерации файла");
        return false;
    }

    if(!outFileGenerator.hasActiveGenFile()) {
        processError(QString("Не выбран путь генерации файла"), "Ошибка генерации файла");
        return false;
    }

    if(outFileGenerator.generate(jsonArr, outFileGenerator.getCurrentGenFilePath())) {
        statusBar()->showMessage("Файл успешно сгенерирован", 5000);
    } else {
        processError(outFileGenerator.getLastError(), "Ошибка генерации файла");
        return false;
    }

    return true;
}

void MainWindow::addPlusRow() {
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);

    // Объединить все столбцы в одну ячейку
    ui->tableWidget->setSpan(row, 0, 1, 9); // row, column, rowSpan, columnSpan

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

    ui->tableWidget->setItem(row, 0, item);
}

void MainWindow::onSelectionChanged(){

}

// Реализация слота для клика по ячейке
void MainWindow::onCellClicked(int row, int col)
{
    if (row == ui->tableWidget->rowCount() - 1) {
        // Клик по последней строке - вставляем новую ПЕРЕД ней
        ui->tableWidget->insertRow(row);
    }
}

void MainWindow::processError(const QString& message, const QString& title) {
    statusBar()->showMessage(title, 5000);
    qCritical() << message;
    QMessageBox::critical(this, title, message);
}
