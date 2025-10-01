#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
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

    // Статус бар
    statusBar()->showMessage("Готов");

    // Размер окна
    resize(1200, 1000);
    setWindowTitle("modbusEdit");

    ui->tableWidget->setColumnCount(9);

    // Заголовки столбцов
    QStringList headers = {
        "Название группы параметров / параметра",
        "Тип доступа", "Тип данных", "Коэффициент",
        "Ед. изм.", "Диапазон значений",
        "Адрес (дес.)", "Адрес (hex.)", "Примечание"
    };
    ui->tableWidget->setHorizontalHeaderLabels(headers);

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

    //Настройка стиля
    ui->tableWidget->setStyleSheet(
        "QHeaderView::section {"
        "    border: 1px solid black;"
        "}"
        );


    //Читаю файл с профилем
    if(!readProfile()) {
        QMessageBox::critical(nullptr,
                              "Ошибка",
                              "Не удалось открыть файл profile.json\n");
    }

    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);

    connect(ui->tableWidget, &QTableWidget::cellClicked,
            this, &MainWindow::onCellClicked);

    // Диагностика
    qDebug() << "Qt version:" << QT_VERSION_STR;
    qDebug() << "=== Проверка настроек таблицы ===";
    qDebug() << "SelectionMode:" << ui->tableWidget->selectionMode();
    qDebug() << "SelectionBehavior:" << ui->tableWidget->selectionBehavior();

}

bool MainWindow::readProfile() {

    // Ищем файл сначала рядом с исполняемым файлом, потом в исходной директории
    QString profilePath;
    QStringList searchPaths = {
        QApplication::applicationDirPath() + "/profile.json",  // рядом с .exe
        QApplication::applicationDirPath() + "/../../../../../profile.json"  // в исходной директории
    };

    for (const QString& path : searchPaths) {
        if (QFile::exists(path)) {
            profilePath = path;
            break;
        }
    }

    if (profilePath.isEmpty()) {
        qDebug() << "Файл profile.json не найден!";
        return false;
    }

    qDebug() << "Найден файл profile.json:" << profilePath;
    QFile profileFile{profilePath};
    if(!profileFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Файл не открылся!";
        qDebug() << "Ошибка:" << profileFile.errorString();
        return false;
    }

    qDebug() << "Файл УСПЕШНО открыт!";

    QByteArray byteArr = profileFile.readAll();
    profileFile.close();

    QJsonDocument doc = QJsonDocument::fromJson(byteArr);
    QJsonArray arr = doc.array();

    int rowCounter = 0;
    for(const QJsonValue &val : std::as_const(arr)) {

        // Добавляем одну строку
        ui->tableWidget->insertRow(rowCounter);

        // Добавляем данные согласно заголовкам столбцов
        ui->tableWidget->setItem(rowCounter, 0, new QTableWidgetItem( val["groupName" ].toString() )); // Название группы параметров
        ui->tableWidget->setItem(rowCounter, 1, new QTableWidgetItem( val["accessType"].toString() )); // Тип доступа
        ui->tableWidget->setItem(rowCounter, 2, new QTableWidgetItem( val["dataType"  ].toString() )); // Тип данных
        ui->tableWidget->setItem(rowCounter, 3, new QTableWidgetItem( val["gain"      ].toString() )); // Коэффициент
        ui->tableWidget->setItem(rowCounter, 4, new QTableWidgetItem( val["units"     ].toString() )); // Ед. изм.
        ui->tableWidget->setItem(rowCounter, 5, new QTableWidgetItem( val["range"     ].toString() )); // Диапазон значений
        ui->tableWidget->setItem(rowCounter, 6, new QTableWidgetItem( val["adressDec" ].toString() )); // Адрес (дес.)
        ui->tableWidget->setItem(rowCounter, 7, new QTableWidgetItem( val["adressHex" ].toString() )); // Адрес (hex.)
        ui->tableWidget->setItem(rowCounter, 8, new QTableWidgetItem( val["note"      ].toString() )); // Примечание
    }
    return true;
}


void MainWindow::onSelectionChanged(){

    QList<QTableWidgetItem*> selected = ui->tableWidget->selectedItems();
    qDebug() << "Выделено ячеек:" << selected.count();

    for (int i = 0; i < selected.count(); i++) {
        QTableWidgetItem *item = selected.at(i);
        qDebug() << "  - Строка:" << item->row() << "Колонка:" << item->column();
    }
}

// Реализация слота для клика по ячейке
void MainWindow::onCellClicked(int row, int col)
{
    qDebug() << "Clicked - Строка:" << row << "Колонка:" << col;
}
