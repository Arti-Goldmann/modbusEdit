#include "jsonprofilemanager.h"

JsonProfileManager::JsonProfileManager(QWidget* parent) : parent(parent) {}

std::optional<JsonProfileManager::TProfileResult> JsonProfileManager::loadProfile() {
    lastError.clear();

    QString lastDir = getLastDirectory();
    QString profilePath = QFileDialog::getOpenFileName(parent,
                                                       QObject::tr("Open File"),
                                                       lastDir,
                                                       QObject::tr("JSON (*.json)"));

    if (profilePath.isEmpty()) {
        setError("Файл .json не найден");
        return std::nullopt;
    }

    // Нашли файл
    currentProfilePath = profilePath;
    // Сохранить директорию для следующего раза
    saveLastDirectory(currentProfilePath);

    auto dataOpt = readJsonData(profilePath);
    if(!dataOpt.has_value()) {
        return std::nullopt;
    }

    auto baseValuesOpt = readJsonBaseValues(profilePath);
    if(!baseValuesOpt.has_value()) {
        return std::nullopt;
    }

    JsonProfileManager::TProfileResult result;
    result.data = dataOpt.value();
    result.baseValues = baseValuesOpt.value();

    return result;
}

std::optional<QJsonObject> JsonProfileManager::readJsonObj(const QString& filePath) {
    lastError.clear();

    // Открытие файла
    QFile file{filePath};
    if(!file.open(QIODevice::ReadOnly)) {
        setError(QString("Не удалось открыть файл: %1\nОшибка: %2").arg(filePath, file.errorString()));
        return std::nullopt;
    }

    // Чтение файла
    QByteArray byteArr = file.readAll();
    file.close();

    if (byteArr.isEmpty()) {
        setError("Файл .json пустой или не удалось прочитать данные");
        return std::nullopt;
    }

    // Парсинг JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(byteArr, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        setError(QString("Ошибка парсинга JSON:\n%1\nСтрока: %2").arg(parseError.errorString(), QString::number(parseError.offset)));
        return std::nullopt;
    }

    if (!doc.isObject()) {
        setError("JSON файл должен быть объектом");
        return std::nullopt;
    }

    return doc.object();
}

std::optional<QJsonArray> JsonProfileManager::readJsonData(const QString& filePath) {
    lastError.clear();

    auto rootObjOpt = readJsonObj(filePath);
    if(!rootObjOpt.has_value()) {
        return std::nullopt;
    }

    QJsonObject rootObj = rootObjOpt.value();

    // Проверка наличия поля с данными
    if (!rootObj.contains("data") || !rootObj["data"].isArray()) {
        setError("JSON файл должен содержать поле 'data' с массивом данных");
        return std::nullopt;
    }

    return rootObj["data"].toArray();
}

std::optional<QJsonArray> JsonProfileManager::readJsonBaseValues(const QString& filePath) {
    lastError.clear();

    auto rootObjOpt = readJsonObj(filePath);
    if(!rootObjOpt.has_value()) {
        return std::nullopt;
    }

    QJsonObject rootObj = rootObjOpt.value();

    // Проверка наличия поля с данными
    if (!rootObj.contains("baseValues") || !rootObj["baseValues"].isArray()) {
        setError("JSON файл должен содержать поле 'baseValues' с базовыми величинами");
        return std::nullopt;
    }

    return rootObj["baseValues"].toArray();
}

bool JsonProfileManager::saveProfileAs(QTableWidget* tableData, QTableWidget* tableBaseValues){
    lastError.clear();

    QString lastDir = getLastDirectory();
    QString profilePath = QFileDialog::getSaveFileName(parent,
                                                       QObject::tr("Save File"),
                                                       lastDir,
                                                       QObject::tr("JSON (*.json)"));

    if (profilePath.isEmpty()) {
        setError("Не задан путь сохранения профиля");
        return false;
    }

    // Выбрали куда сохранить файл
    currentProfilePath = profilePath;
    // Сохранить директорию для следующего раза
    saveLastDirectory(currentProfilePath);

    //Сохраняем профиль в файл
    //Дошли сюда только в том случае, если пользователь согласился на перезапись, если таковая могла случиться
    return saveProfile(tableData, tableBaseValues);
}


bool JsonProfileManager::saveProfile(QTableWidget* tableData, QTableWidget* tableBaseValues){
    lastError.clear();

    QJsonArray data;
    QJsonArray baseValues;

    //Проходим по всем строкам таблицы, кроме последней, потому что там строка с "+"
    for(int rowCounter = 0; rowCounter < tableData->rowCount() - 1; rowCounter++) {
        //Проходим по всем колонкам таблицы и сохраняем в соответсвующий ключ json
        QJsonObject obj;

        for(int col = 0; col < COLUMN_KEYS.size(); col++) {
            QTableWidgetItem* item = tableData->item(rowCounter, col);
            obj[COLUMN_KEYS[col]] = item ? item->text() : "";
        }

        data.append(obj);
    }

    for(int rowCounter = 0; rowCounter < tableBaseValues->rowCount() - 1; rowCounter++) {
        //Проходим по всем колонкам таблицы и сохраняем в соответсвующий ключ json
        QJsonObject obj;

        for(int col = 0; col < BASE_VALUES_KEYS.size(); col++) {
            QTableWidgetItem* item = tableBaseValues->item(rowCounter, col);
            obj[BASE_VALUES_KEYS[col]] = item ? item->text() : "";
        }

        baseValues.append(obj);
    }

    QString profilePath = currentProfilePath;

    qDebug() << "Текущий путь активного профиля " << profilePath;

    // Открытие файла
    if(!profilePath.isEmpty()) {
        //Пришли сюда после нажатия "Открыть" или из "Сохранить как" и есть путь до файла профиля
        QFile file{profilePath};

        if(!file.open(QIODevice::WriteOnly)) {
            setError(QString("Не удалось открыть файл: %1\nОшибка: %2").arg(profilePath, file.errorString()));
            return false;
        }

        QJsonObject rootObj;
        rootObj["baseValues"] = baseValues;
        rootObj["data"] = data;

        QJsonDocument doc(rootObj);
        QByteArray byteArr = doc.toJson(QJsonDocument::Indented);

        qint64 bytesWritten = file.write(byteArr);
        if(bytesWritten == -1) {
            // Ошибка записи
            setError(QString("Не удалось записать в файл: %1\nОшибка: %2").arg(profilePath, file.errorString()));
            file.close();
            return false;
        }
        if(bytesWritten != byteArr.size()) {
            // Записалось не всё
            setError(QString("В файл профиля записались не все данные: %1\nОшибка: %2").arg(profilePath, file.errorString()));
            file.close();
            return false;
        }

        file.close();
    } else {
        //Если профиль никакой не открыли еще и просто вносили изменения в исходной таблице, то нужно его "Cохранить как"
        return saveProfileAs(tableData, tableBaseValues);
    }

    return true;
}

QString JsonProfileManager::getLastDirectory() const {
    QSettings settings("MPEI", "modbusEdit");
    QString lastDir = settings.value("lastProfileDirectory", QDir::homePath()).toString();
    return lastDir;
}

void JsonProfileManager::saveLastDirectory(const QString& path) {
    QSettings settings("MPEI", "modbusEdit");
    QFileInfo fileInfo(path);
    settings.setValue("lastProfileDirectory", fileInfo.absolutePath());
}

