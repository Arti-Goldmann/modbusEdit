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
    statusBar()->showMessage("Выберете файл профиля");

    // Размер окна
    resize(1200, 1000);
    setWindowTitle("modbusEdit");

    ui->tableWidget->setColumnCount(9);
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

    connect(ui->open, &QAction::triggered,
            this, &MainWindow::readProfile);

    connect(ui->save, &QAction::triggered,
            this, &MainWindow::saveProfile);

    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);

    connect(ui->tableWidget, &QTableWidget::cellClicked,
            this, &MainWindow::onCellClicked);
}

bool MainWindow::readProfile() {
    QString errorMessage;

    // Загрузить последний путь из настроек
    QSettings settings("MPEI", "modbusEdit");
    QString lastDir = settings.value("lastDirectory", QDir::homePath()).toString();

    QString profilePath = QFileDialog::getOpenFileName(this, tr("Open File"),
                                                    lastDir,
                                                    tr("JSON (*.json)"));
    
    if (profilePath.isEmpty()) {
        errorMessage = "Файл profile.json не найден!";
        qCritical() << errorMessage;
        QMessageBox::critical(this, "Ошибка загрузки профиля", errorMessage);
        return false;
    }

    // Нашли файл
    activeProfilePath = profilePath;
    // Сохранить директорию для следующего раза
    QFileInfo fileInfo(profilePath);
    settings.setValue("lastDirectory", fileInfo.absolutePath());

    qDebug() << "Найден файл profile.json:" << profilePath;
    
    // Открытие файла
    QFile profileFile{profilePath};
    if(!profileFile.open(QIODevice::ReadOnly)) {
        errorMessage = QString("Не удалось открыть файл: %1\nОшибка: %2")
                          .arg(profilePath, profileFile.errorString());
        qCritical() << errorMessage;
        QMessageBox::critical(this, "Ошибка загрузки профиля", errorMessage);
        return false;
    }

    // Чтение файла
    QByteArray byteArr = profileFile.readAll();
    profileFile.close();
    
    if (byteArr.isEmpty()) {
        errorMessage = "Файл profile.json пустой или не удалось прочитать данные";
        qCritical() << errorMessage;
        QMessageBox::critical(this, "Ошибка загрузки профиля", errorMessage);
        return false;
    }

    // Парсинг JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(byteArr, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        errorMessage = QString("Ошибка парсинга JSON:\n%1\nСтрока: %2")
                          .arg(parseError.errorString(), QString::number(parseError.offset));
        qCritical() << errorMessage;
        QMessageBox::critical(this, "Ошибка загрузки профиля", errorMessage);
        return false;
    }
    
    if (!doc.isArray()) {
        errorMessage = "JSON файл должен содержать массив объектов";
        qCritical() << errorMessage;
        QMessageBox::critical(this, "Ошибка загрузки профиля", errorMessage);
        return false;
    }

    QJsonArray arr = doc.array();
    
    if (arr.isEmpty()) {
        errorMessage = "JSON массив пустой";
        qWarning() << errorMessage;
        QMessageBox::warning(this, "Предупреждение", errorMessage);
        return true; // Не критическая ошибка
    }

    // Очищаем таблицу перед загрузкой
    ui->tableWidget->setRowCount(0);
    
    int rowCounter = 0;
    for(const QJsonValue &val : std::as_const(arr)) {
        
        QJsonObject obj = val.toObject();
        
        // Добавляем строку в таблицу
        ui->tableWidget->insertRow(rowCounter);

        //Получаем строку из объекта json и кладем в соответсвующую колонку
        for(int col = 0; col < COLUMN_KEYS.size(); col++) {
            QString str = obj[COLUMN_KEYS[col]].toString();
            ui->tableWidget->setItem(rowCounter, col, new QTableWidgetItem(str));
        }
        rowCounter++;
    }

    addPlusRow(); //Добавляем последнюю строку с плюсиком
    
    qInfo() << QString("Успешно загружен профиль: %1 строк").arg(rowCounter);
    return true;
}

bool MainWindow::saveProfile(){
    QString errorMessage;
    QJsonArray jsonArr;

    //Проходим по всем строкам таблицы, кроме последней, потому что там строка с "+"
    for(int rowCounter = 0; rowCounter < ui->tableWidget->rowCount() - 1; rowCounter++) {
        //Проходим по всем колонкам таблицы и сохраняем в соответсвующий ключ json
        QJsonObject obj;

        for(int col = 0; col < COLUMN_KEYS.size(); col++) {
            QTableWidgetItem* item = ui->tableWidget->item(rowCounter, col);
            obj[COLUMN_KEYS[col]] = item ? item->text() : "";
        }

        jsonArr.append(obj);
    }

    QString profilePath = activeProfilePath;

    // Открытие файла
    if(profilePath != "") { //Открыли профиль и есть путь до него
        QFile profileFile{profilePath};
        if(!profileFile.open(QIODevice::WriteOnly)) {
            errorMessage = QString("Не удалось открыть файл: %1\nОшибка: %2")
                               .arg(profilePath, profileFile.errorString());
            qCritical() << errorMessage;
            QMessageBox::critical(this, "Ошибка загрузки профиля", errorMessage);
            return false;
        }

        QJsonDocument doc(jsonArr);
        QByteArray byteArr = doc.toJson(QJsonDocument::Indented);

        qint64 bytesWritten = profileFile.write(byteArr);
        if(bytesWritten == -1) {
            // Ошибка записи
            errorMessage = QString("Не удалось записать в файл: %1\nОшибка: %2")
                               .arg(profilePath, profileFile.errorString());
            qCritical() << errorMessage;
            QMessageBox::critical(this, "Ошибка сохранения профиля", errorMessage);
        }
        if(bytesWritten != byteArr.size()) {
            // Записалось не всё
            errorMessage = QString("В файл профиля записались не все данные: %1\nОшибка: %2")
                               .arg(profilePath, profileFile.errorString());
            qCritical() << errorMessage;
            QMessageBox::critical(this, "Ошибка сохранения профиля", errorMessage);
        }

        profileFile.close();
    } else {
        //TODO: а если ничего еще не открыли, то надо создать
        return false;
    }

    statusBar()->showMessage("Файл успешно сохранен", 5000);
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
    if (row == ui->tableWidget->rowCount() - 1) {
        // Клик по последней строке - вставляем новую ПЕРЕД ней
        ui->tableWidget->insertRow(row);
    }

    qDebug() << "Clicked - Строка:" << row << "Колонка:" << col;
}
