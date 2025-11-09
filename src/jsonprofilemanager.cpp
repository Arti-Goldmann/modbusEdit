#include "jsonprofilemanager.h"

JsonProfileManager::JsonProfileManager(QWidget* parent) : parent(parent) {}

std::optional<JsonProfileManager::TProfileResult> JsonProfileManager::readProfileFormPath(const QString& profilePath) {
    lastError.clear();

    if (profilePath.isEmpty()) {
        setError("Файл .json не найден");
        return std::nullopt;
    }

    // Нашли файл
    currentProfilePath = profilePath;
    // Сохранить директорию для следующего раза
    saveLastDirectory(currentProfilePath);

    auto dataOpt = readJsonArr(profilePath, Constants::JsonKeys::Data::ARRAY_NAME);
    if(!dataOpt.has_value()) {
        return std::nullopt;
    }

    auto baseValuesOpt = readJsonArr(profilePath, Constants::JsonKeys::BaseValues::ARRAY_NAME);
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


std::optional<QJsonArray> JsonProfileManager::readJsonArr(const QString& filePath, const QString& field) {
    lastError.clear();

    auto rootObjOpt = readJsonObj(filePath);
    if(!rootObjOpt.has_value()) {
        return std::nullopt;
    }

    QJsonObject rootObj = rootObjOpt.value();

    // Проверка наличия поля с данными
    if (!rootObj.contains(field) || !rootObj[field].isArray()) {
        setError(QString("JSON файл должен содержать поле %1").arg(field));
        return std::nullopt;
    }

    return rootObj[field].toArray();
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

QJsonArray JsonProfileManager::tableToJsonArray(QTableWidget* table, const QStringList& mainKeys, const QStringList& additionalKeys) {
    QJsonArray result;

    // Проходим по всем строкам, кроме последней (строка с "+")
    for(int row = 0; row < table->rowCount() - 1; row++) {
        QJsonObject obj;
        for(int col = 0; col < table->columnCount(); col++) {
            QTableWidgetItem* item = table->item(row, col);
            obj[mainKeys[col]] = item ? item->text() : "";
        }

        //TODO: нвдо сделать ошибки, если условия не выполняеются

        //Дополнительные ключи json
        if(!additionalKeys.isEmpty()) {
            QTableWidgetItem* rootItem = table->item(row, 0); //Инфа в 0 ячейке
            if(rootItem) {
                QVariant data = rootItem->data(Qt::UserRole);
                QString rowType = data.isValid() ? data.toString() : "";

                if(!additionalKeys.contains(Constants::JsonKeys::Data::PARAM_TYPE)) return {};
                if(!additionalKeys.contains(Constants::JsonKeys::Data::USER_CODE_R)) return {};
                if(!additionalKeys.contains(Constants::JsonKeys::Data::USER_CODE_W)) return {};

                obj[Constants::JsonKeys::Data::PARAM_TYPE] = rowType;

                if (rowType == Constants::ParamType::USER) {
                    // Получаем код пользователя
                    data = rootItem->data(Qt::UserRole + 1 + Constants::UserCodeOffsetInTable::R);
                    QString userCode_R = data.isValid() ? data.toString() : "";
                    data = rootItem->data(Qt::UserRole + 1 + Constants::UserCodeOffsetInTable::W);
                    QString userCode_W = data.isValid() ? data.toString() : "";
                    obj[Constants::JsonKeys::Data::USER_CODE_R] = userCode_R;
                    obj[Constants::JsonKeys::Data::USER_CODE_W] = userCode_W;
                    obj[Constants::JsonKeys::Data::VAR_NAME] = "";
                } else if(rowType == Constants::ParamType::COMMON) {
                    obj[Constants::JsonKeys::Data::USER_CODE_R] = "";
                    obj[Constants::JsonKeys::Data::USER_CODE_W] = "";
                } else if(rowType == Constants::ParamType::TITLE) {
                    obj[Constants::JsonKeys::Data::USER_CODE_R] = "";
                    obj[Constants::JsonKeys::Data::USER_CODE_W] = "";
                    // Текст заголовка из первой ячейки сохраняем в GROUP_NAME
                    obj[Constants::JsonKeys::Data::GROUP_NAME] = rootItem->text();
                }
            }
        }
        result.append(obj);
    }

    return result;
}

bool JsonProfileManager::saveProfile(QTableWidget* tableData, QTableWidget* tableBaseValues){
    lastError.clear();

    QString profilePath = currentProfilePath;

    // Открытие файла
    if(!profilePath.isEmpty()) {
        //Пришли сюда после нажатия "Открыть" или из "Сохранить как" и есть путь до файла профиля
        QFile file{profilePath};

        if(!file.open(QIODevice::WriteOnly)) {
            setError(QString("Не удалось открыть файл: %1\nОшибка: %2").arg(profilePath, file.errorString()));
            return false;
        }

        QJsonObject rootObj;
        rootObj[Constants::JsonKeys::BaseValues::ARRAY_NAME] = tableToJsonArray(tableBaseValues, BASE_VALUES_ARR_KEYS);
        rootObj[Constants::JsonKeys::Data::ARRAY_NAME] = tableToJsonArray(tableData, DATA_MAIN_KEYS, DATA_HIDDEN_KEYS);

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

