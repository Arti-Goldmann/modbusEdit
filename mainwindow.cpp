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

    connect(ui->open, &QAction::triggered,
            this, &MainWindow::readProfile);

    connect(ui->save, &QAction::triggered,
            this, &MainWindow::saveProfile);

    connect(ui->saveAs, &QAction::triggered,
            this, &MainWindow::saveAsProfile);

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
        showProfileError("Файл profile.json не найден!", "Ошибка загрузки профиля");
        return false;
    }

    // Нашли файл
    activeProfilePath = profilePath;

    // Сохранить директорию для следующего раза
    QFileInfo fileInfo(profilePath);
    settings.setValue("lastDirectory", fileInfo.absolutePath());

    ui->fileLabel->setText(QString("Файл: %1").arg(fileInfo.fileName()));

    qDebug() << "Найден файл profile.json:" << profilePath;

    // Открытие файла
    QFile profileFile{profilePath};
    if(!profileFile.open(QIODevice::ReadOnly)) {
        showProfileError(QString("Не удалось открыть файл: %1\nОшибка: %2")
                          .arg(profilePath, profileFile.errorString()), "Ошибка загрузки профиля");
        return false;
    }

    // Чтение файла
    QByteArray byteArr = profileFile.readAll();
    profileFile.close();

    if (byteArr.isEmpty()) {
        showProfileError("Файл profile.json пустой или не удалось прочитать данные", "Ошибка загрузки профиля");
        return false;
    }

    // Парсинг JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(byteArr, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        showProfileError(QString("Ошибка парсинга JSON:\n%1\nСтрока: %2")
                          .arg(parseError.errorString(), QString::number(parseError.offset)), "Ошибка загрузки профиля");
        return false;
    }

    if (!doc.isArray()) {
        showProfileError("JSON файл должен содержать массив объектов", "Ошибка загрузки профиля");
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

    statusBar()->showMessage("Файл успешно открыт", 5000);
    return true;
}

void MainWindow::showProfileError(const QString& message, const QString& title) {
    statusBar()->showMessage(title, 5000);
    qCritical() << message;
    QMessageBox::critical(this, title, message);
}

bool MainWindow::saveAsProfile(){

    // Загрузить последний путь из настроек
    QSettings settings("MPEI", "modbusEdit");
    QString lastDir = settings.value("lastDirectory", QDir::homePath()).toString();

    QString profilePath = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                       lastDir,
                                                       tr("JSON (*.json)"));

    if (profilePath.isEmpty()) {
        showProfileError("Файл профиля не сохранен", "Ошибка сохранения профиля");
        return false;
    }

    // Выбрали куда сохранить файл
    activeProfilePath = profilePath;

    // Сохранить директорию для следующего раза
    QFileInfo fileInfo(profilePath);
    settings.setValue("lastDirectory", fileInfo.absolutePath());

    ui->fileLabel->setText(QString("Файл: %1").arg(fileInfo.fileName()));

    //Сохраняем профиль в файл
    //Дошли сюда только в том случае, если пользователь согласился на перезапись
    return saveProfile();
}


bool MainWindow::saveProfile(){
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
    if(profilePath != "") {
        //Пришли сюда после нажатия "Открыть" или из "Сохранить как" и есть путь до файла профиля
        QFile profileFile{profilePath};

        if(!profileFile.open(QIODevice::WriteOnly)) {
            showProfileError(QString("Не удалось открыть файл: %1\nОшибка: %2")
                               .arg(profilePath, profileFile.errorString()), "Ошибка сохранения профиля");
            return false;
        }

        QJsonDocument doc(jsonArr);
        QByteArray byteArr = doc.toJson(QJsonDocument::Indented);

        qint64 bytesWritten = profileFile.write(byteArr);
        if(bytesWritten == -1) {
            // Ошибка записи
            showProfileError(QString("Не удалось записать в файл: %1\nОшибка: %2")
                               .arg(profilePath, profileFile.errorString()), "Ошибка сохранения профиля");
            profileFile.close();
            return false;
        }
        if(bytesWritten != byteArr.size()) {
            // Записалось не всё
            showProfileError(QString("В файл профиля записались не все данные: %1\nОшибка: %2")
                               .arg(profilePath, profileFile.errorString()), "Ошибка сохранения профиля");
            profileFile.close();
            return false;
        }

        profileFile.close();
    } else {
        //Если профиль никакой не открыли еще и просто вносили изменения в исходной таблице, то нужно его "Cохранить как"
        saveAsProfile();
        return true;
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
