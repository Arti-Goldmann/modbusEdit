#include "profileoperations.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QApplication>

ProfileOperations::ProfileOperations(QWidget* parentWidget,
                                     JsonProfileManager* profileManager,
                                     TableManager* tableManager,
                                     QObject *parent)
    : QObject(parent)
    , parentWidget(parentWidget)
    , profileManager(profileManager)
    , tableManager(tableManager)
    , profileWatcher(nullptr)
    , progressDialog(nullptr)
    , currentDataTable(nullptr)
    , currentBaseValuesTable(nullptr)
{
}

void ProfileOperations::loadProfile(QTableWidget* dataTable, QTableWidget* baseValuesTable) {
    //Выбор файла
    QString lastDir = profileManager->getLastDirectory();
    QString profilePath = QFileDialog::getOpenFileName(parentWidget,
                                                       QObject::tr("Open File"),
                                                       lastDir,
                                                       QObject::tr("JSON (*.json)"));

    if (profilePath.isEmpty()) {
        // Пользователь отменил выбор
        return;
    }

    profileManager->setProfilePath(profilePath);

    // Сохраняем таблицы для использования в onProfileLoadFinished
    currentDataTable = dataTable;
    currentBaseValuesTable = baseValuesTable;

    // Создаем диалог прогресса
    progressDialog = new QProgressDialog("Загрузка профиля...", "Отмена", 0, 0, parentWidget);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setMinimumDuration(0); // Показать сразу
    progressDialog->setCancelButton(nullptr); // Убираем кнопку отмены
    progressDialog->show();

    // Создаем watcher для отслеживания завершения
    profileWatcher = new QFutureWatcher<std::optional<JsonProfileManager::TProfileResult>>(this);

    // Подключаем сигнал завершения
    connect(profileWatcher, &QFutureWatcher<std::optional<JsonProfileManager::TProfileResult>>::finished,
            this, &ProfileOperations::onProfileLoadFinished);

    // Запускаем загрузку в фоновом потоке
    QFuture<std::optional<JsonProfileManager::TProfileResult>> future = QtConcurrent::run(
        loadProfileInBackground,
        profileManager
        );

    profileWatcher->setFuture(future);
}

std::optional<JsonProfileManager::TProfileResult> ProfileOperations::loadProfileInBackground(JsonProfileManager* manager) {
    // Эта функция выполняется в отдельном потоке!
    // Здесь НЕ ДОЛЖНО быть работы с UI!
    return manager->readProfile();
}

void ProfileOperations::onProfileLoadFinished() {
    // Получаем результат из фонового потока
    auto profileResultOpt = profileWatcher->result();

    if (!profileResultOpt.has_value()) {
        // Закрываем диалог при ошибке
        progressDialog->close();
        progressDialog->deleteLater();
        profileWatcher->deleteLater();

        emit errorOccurred(profileManager->getLastError(), "Ошибка загрузки профиля");
        return;
    }

    // Обновляем текст диалога перед заполнением таблиц
    progressDialog->setLabelText("Заполнение таблицы...");

    QJsonArray data = profileResultOpt.value().data;
    QJsonArray baseValues = profileResultOpt.value().baseValues;

    tableManager->fillTable(currentDataTable, data, profileManager->DATA_MAIN_KEYS, true);
    tableManager->fillTable(currentBaseValuesTable, baseValues, profileManager->BASE_VALUES_ARR_KEYS);

    // Закрываем диалог прогресса ПОСЛЕ fillTable
    progressDialog->close();
    progressDialog->deleteLater();

    // Очищаем watcher
    profileWatcher->deleteLater();

    // Уведомляем об успешной загрузке
    emit profileLoaded(profileManager->getCurrentProfilePath());
}

bool ProfileOperations::saveProfile(QTableWidget* dataTable, QTableWidget* baseValuesTable) {
    return saveProfileHandler(dataTable, baseValuesTable, false);
}

bool ProfileOperations::saveProfileAs(QTableWidget* dataTable, QTableWidget* baseValuesTable) {
    return saveProfileHandler(dataTable, baseValuesTable, true);
}

bool ProfileOperations::saveProfileHandler(QTableWidget* dataTable,
                                          QTableWidget* baseValuesTable,
                                          bool isSaveAs) {
    bool result = isSaveAs ? profileManager->saveProfileAs(dataTable, baseValuesTable) :
                             profileManager->saveProfile(dataTable, baseValuesTable);

    if(result) {
        emit profileSaved(profileManager->getCurrentProfilePath());
    } else {
        emit errorOccurred(profileManager->getLastError(), "Ошибка сохранения файла");
        return false;
    }

    return true;
}
