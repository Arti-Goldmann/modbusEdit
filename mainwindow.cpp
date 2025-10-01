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


    for(int i=0; i < 100; i++) {

        // Добавляем одну строку
        ui->tableWidget->insertRow(i);

        // Добавляем данные согласно заголовкам столбцов
        ui->tableWidget->setItem(i, 0, new QTableWidgetItem("Температура процессора")); // Название группы параметров
        ui->tableWidget->setItem(i, 1, new QTableWidgetItem("R"));                    // Тип доступа
        ui->tableWidget->setItem(i, 2, new QTableWidgetItem("INT16"));                // Тип данных
        ui->tableWidget->setItem(i, 3, new QTableWidgetItem("0.1"));                  // Коэффициент
        ui->tableWidget->setItem(i, 4, new QTableWidgetItem("°C"));                   // Ед. изм.
        ui->tableWidget->setItem(i, 5, new QTableWidgetItem("-40...+85"));            // Диапазон значений
        ui->tableWidget->setItem(i, 6, new QTableWidgetItem("100"));                  // Адрес (дес.)
        ui->tableWidget->setItem(i, 7, new QTableWidgetItem("0x64"));                 // Адрес (hex.)
        ui->tableWidget->setItem(i, 8, new QTableWidgetItem("Внутренний датчик"));    // Примечание
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
