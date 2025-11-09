#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , outFileGenerator(parent)
    , jsonProfileManager(parent)
    , tableManager(new TableManager(this))
    , profileOperations(nullptr)
    , accessTypeDelegate(nullptr)
    , dataTypeDelegate(nullptr)
    , baseValueDelegate(nullptr)
    , iqFormatDelegate(nullptr)
    , hasUnsavedChanges(false)
    , fileGenWatcher(nullptr)
    , progressDialog(nullptr)
{
    ui->setupUi(this);

    // Создаем ProfileOperations после создания tableManager
    profileOperations = new ProfileOperations(this, &jsonProfileManager, tableManager, this);

    setupUI();
    setupDelegates();
    connectSignals();
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
    ui->tableWidgetDataValues->setColumnCount(Constants::TableHeaders::DATA_TABLE().size());
    ui->tableWidgetDataValues->setHorizontalHeaderLabels(Constants::TableHeaders::DATA_TABLE());
    ui->tableWidgetDataValues->setContextMenuPolicy(Qt::CustomContextMenu);

    // Настройка ширины столбцов
    ui->tableWidgetDataValues->setColumnWidth(0, 300);  // Название группы параметров
    ui->tableWidgetDataValues->setColumnWidth(1, 100);  // Тип доступа
    ui->tableWidgetDataValues->setColumnWidth(2, 100);  // Тип данных
    ui->tableWidgetDataValues->setColumnWidth(3, 100);  // Коэффициент
    ui->tableWidgetDataValues->setColumnWidth(4, 80);   // Адрес (дес.)
    ui->tableWidgetDataValues->setColumnWidth(5, 80);   // Адрес (hex.)
    ui->tableWidgetDataValues->setColumnWidth(6, 200);  // Переменнная
    ui->tableWidgetDataValues->setColumnWidth(7, 200);  // Базовая величина
    ui->tableWidgetDataValues->setColumnWidth(8, 150);  // Примечание

    tableManager->setupTable(ui->tableWidgetDataValues);
    tableManager->addPlusRow(ui->tableWidgetDataValues); //Добавляем последнюю строку с плюсиком

    //Настройки таблицы с базовыми величинами
    ui->tableWidgetBaseValues->setColumnCount(Constants::TableHeaders::BASE_VALUES_ARR().size());
    ui->tableWidgetBaseValues->setHorizontalHeaderLabels(Constants::TableHeaders::BASE_VALUES_ARR());
    ui->tableWidgetBaseValues->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->tableWidgetBaseValues->setColumnWidth(0, 300);
    ui->tableWidgetBaseValues->setColumnWidth(1, 70);
    ui->tableWidgetBaseValues->setColumnWidth(2, 70);
    ui->tableWidgetBaseValues->setColumnWidth(3, 200);
    ui->tableWidgetBaseValues->setColumnWidth(4, 300);

    tableManager->setupTable(ui->tableWidgetBaseValues);
    tableManager->addPlusRow(ui->tableWidgetBaseValues); //Добавляем последнюю строку с плюсиком
}

void MainWindow::setupDelegates() {
    // Настройка делегатов для комбобоксов
    QStringList accessTypes = Constants::AccessType::toStringList();
    accessTypeDelegate = new ComboBoxDelegate(accessTypes, this);

    QStringList dataTypes = Constants::DataType::toStringList();
    dataTypeDelegate = new ComboBoxDelegate(dataTypes, this);

    baseValueDelegate = new DynamicComboBoxDelegate(ui->tableWidgetBaseValues, 0, this);

    // Устанавливаем делегаты для соответствующих столбцов
    ui->tableWidgetDataValues->setItemDelegateForColumn(1, accessTypeDelegate);  // Тип доступа
    ui->tableWidgetDataValues->setItemDelegateForColumn(2, dataTypeDelegate);    // Тип данных
    ui->tableWidgetDataValues->setItemDelegateForColumn(7, baseValueDelegate);   // Базовая величина

    // Создаем делегат для формата IQ
    QStringList iqFormats;
    for(int i = 16; i >= 0; --i) {
        iqFormats << QString("%1.%2").arg(i).arg(16 - i);
    }
    iqFormatDelegate = new ComboBoxDelegate(iqFormats, this);

    // Устанавливаем делегат для столбца "Формат IQ"
    ui->tableWidgetBaseValues->setItemDelegateForColumn(2, iqFormatDelegate);
}

void MainWindow::connectSignals() {
    // Меню и кнопки
    connect(ui->toolBtnGenPath, &QPushButton::clicked, this, &MainWindow::onSetGenerationPath);
    connect(ui->startGenBtn, &QPushButton::clicked, this, &MainWindow::onStartGeneration);
    connect(ui->open, &QAction::triggered, this, &MainWindow::onLoadProfile);
    connect(ui->save, &QAction::triggered, this, &MainWindow::onSaveProfile);
    connect(ui->saveAs, &QAction::triggered, this, &MainWindow::onSaveProfileAs);

    // Сигналы от TableManager
    connect(tableManager, &TableManager::dataModified, this, &MainWindow::onDataModified);
    connect(tableManager, &TableManager::errorOccurred, this, &MainWindow::processError);

    // Сигналы от ProfileOperations
    connect(profileOperations, &ProfileOperations::profileLoaded, this, &MainWindow::onProfileLoaded);
    connect(profileOperations, &ProfileOperations::profileSaved, this, &MainWindow::onProfileSaved);
    connect(profileOperations, &ProfileOperations::errorOccurred, this, &MainWindow::processError);

    // Таблица данных
    connect(ui->tableWidgetDataValues, &QTableWidget::cellClicked, this,
            [this](int row, int col) {
                tableManager->handleCellClicked(ui->tableWidgetDataValues, row, col);
            });

    connect(ui->tableWidgetDataValues, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int col) {
                tableManager->handleCellDoubleClicked(ui->tableWidgetDataValues, row, col);
            });

    connect(ui->tableWidgetDataValues, &QTableWidget::itemChanged, this,
            [this](QTableWidgetItem* item) {
                tableManager->handleTableDataChanged(ui->tableWidgetDataValues, item);
            });

    connect(ui->tableWidgetDataValues, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint &pos) {
                tableManager->handleContextMenu(ui->tableWidgetDataValues, pos);
            });

    // Таблица базовых величин
    connect(ui->tableWidgetBaseValues, &QTableWidget::cellClicked, this,
            [this](int row, int col) {
                tableManager->handleCellClicked(ui->tableWidgetBaseValues, row, col);
            });

    connect(ui->tableWidgetBaseValues, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint &pos) {
                tableManager->handleContextMenu(ui->tableWidgetBaseValues, pos);
            });

    // Подключаем автообновление базовых величин
    connect(ui->tableWidgetBaseValues, &QTableWidget::itemChanged, this, &MainWindow::onBaseValuesChanged);
}

void MainWindow::onLoadProfile() {
    profileOperations->loadProfile(ui->tableWidgetDataValues, ui->tableWidgetBaseValues);
}

void MainWindow::onSaveProfile() {
    profileOperations->saveProfile(ui->tableWidgetDataValues, ui->tableWidgetBaseValues);
}

void MainWindow::onSaveProfileAs() {
    profileOperations->saveProfileAs(ui->tableWidgetDataValues, ui->tableWidgetBaseValues);
}

bool MainWindow::onSetGenerationPath() {
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

void MainWindow::onStartGeneration(){
    // Создаем диалог прогресса
    progressDialog = new QProgressDialog("Сохранение файла...", QString(), 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0);
    progressDialog->setCancelButton(nullptr);
    progressDialog->show();

    // Принудительно обрабатываем события, чтобы диалог отрисовался
    QApplication::processEvents();

    //Нужно сначала сохранить изменения в json перед тем как генерировать
    if(!profileOperations->saveProfile(ui->tableWidgetDataValues, ui->tableWidgetBaseValues)) {
        progressDialog->close();
        progressDialog->deleteLater();
        return;
    }

    progressDialog->setLabelText("Чтение профиля...");
    QApplication::processEvents();

    auto resultReadJsonOpt = jsonProfileManager.readProfile();

    if (!resultReadJsonOpt.has_value()) {
        progressDialog->close();
        progressDialog->deleteLater();
        processError(jsonProfileManager.getLastError(), "Ошибка генерации файла");
        return;
    }

    QJsonArray data = resultReadJsonOpt.value().data;
    QJsonArray baseValues = resultReadJsonOpt.value().baseValues;

    if(data.isEmpty() || baseValues.isEmpty()) {
        progressDialog->close();
        progressDialog->deleteLater();
        processError(QString("Пустой профиль: %1").arg(jsonProfileManager.getCurrentProfilePath()), "Ошибка генерации файла");
        return;
    }

    if(!outFileGenerator.hasActiveGenFile()) {
        progressDialog->close();
        progressDialog->deleteLater();
        processError(QString("Не выбран путь генерации файла"), "Ошибка генерации файла");
        return;
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
}

bool MainWindow::genFileInBackground(OutFileGenerator* generator, QJsonArray data, QJsonArray baseValues) {
    // Эта функция выполняется в отдельном потоке!
    // Здесь НЕ ДОЛЖНО быть работы с UI!
    return generator->generate(data, baseValues);
}

void MainWindow::onGenerationFinished(){
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
    }
}

void MainWindow::onDataModified() {
    setModified(true);
}

void MainWindow::onProfileLoaded(const QString& profilePath) {
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

    // Сбрасываем флаг изменений после загрузки профиля
    setModified(false);
}

void MainWindow::onProfileSaved(const QString& profilePath) {
    QFileInfo fileInfo(profilePath);
    ui->fileLabel->setText(QString("Файл профиля: %1").arg(fileInfo.fileName()));
    statusBar()->showMessage("Файл успешно сохранен", 5000);
    setModified(false);  // Сбрасываем флаг изменений после успешного сохранения
}

void MainWindow::onBaseValuesChanged() {
    // Пересоздаем делегат для базовых величин с обновленными данными
    delete baseValueDelegate;
    baseValueDelegate = new DynamicComboBoxDelegate(ui->tableWidgetBaseValues, 0, this);
    ui->tableWidgetDataValues->setItemDelegateForColumn(Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::BASE_VALUE), baseValueDelegate);

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
        return profileOperations->saveProfile(ui->tableWidgetDataValues, ui->tableWidgetBaseValues);
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

void MainWindow::processError(const QString& message, const QString& title) {
    statusBar()->showMessage(title, 5000);
    qCritical() << message;
    QMessageBox::critical(this, title, message);
}
