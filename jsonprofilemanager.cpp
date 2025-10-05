#include "jsonprofilemanager.h"

JsonProfileManager::JsonProfileManager(QWidget* parent) : parent(parent) {}

std::optional<QJsonArray> JsonProfileManager::loadProfile() {
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

    return readJsonFile(profilePath);
}


std::optional<QJsonArray> JsonProfileManager::readJsonFile(const QString& filePath) {
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

    if (!doc.isArray()) {
        setError("JSON файл должен содержать массив объектов");
        return std::nullopt;
    }

    return doc.array();
}


bool JsonProfileManager::saveProfileAs(QTableWidget* table){
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

    //TODO: это пересети в MainWindow
    //ui->fileLabel->setText(QString("Файл: %1").arg(fileInfo.fileName()));

    //Сохраняем профиль в файл
    //Дошли сюда только в том случае, если пользователь согласился на перезапись, если таковая могла случиться
    return saveProfile(table);
}


bool JsonProfileManager::saveProfile(QTableWidget* table){
    lastError.clear();

    QJsonArray jsonArr;

    //Проходим по всем строкам таблицы, кроме последней, потому что там строка с "+"
    for(int rowCounter = 0; rowCounter < table->rowCount() - 1; rowCounter++) {
        //Проходим по всем колонкам таблицы и сохраняем в соответсвующий ключ json
        QJsonObject obj;

        for(int col = 0; col < COLUMN_KEYS.size(); col++) {
            QTableWidgetItem* item = table->item(rowCounter, col);
            obj[COLUMN_KEYS[col]] = item ? item->text() : "";
        }

        jsonArr.append(obj);
    }

    QString profilePath = currentProfilePath;

    qDebug() << "Текущий путь активного профиля " << profilePath;

    // Открытие файла
    if(!profilePath.isEmpty()) {
        //Пришли сюда после нажатия "Открыть" или из "Сохранить как" и есть путь до файла профиля
        QFile profileFile{profilePath};

        if(!profileFile.open(QIODevice::WriteOnly)) {
            setError(QString("Не удалось открыть файл: %1\nОшибка: %2").arg(profilePath, profileFile.errorString()));
            return false;
        }

        QJsonDocument doc(jsonArr);
        QByteArray byteArr = doc.toJson(QJsonDocument::Indented);

        qint64 bytesWritten = profileFile.write(byteArr);
        if(bytesWritten == -1) {
            // Ошибка записи
            setError(QString("Не удалось записать в файл: %1\nОшибка: %2").arg(profilePath, profileFile.errorString()));
            profileFile.close();
            return false;
        }
        if(bytesWritten != byteArr.size()) {
            // Записалось не всё
            setError(QString("В файл профиля записались не все данные: %1\nОшибка: %2").arg(profilePath, profileFile.errorString()));
            profileFile.close();
            return false;
        }

        profileFile.close();
    } else {
        //Если профиль никакой не открыли еще и просто вносили изменения в исходной таблице, то нужно его "Cохранить как"
        return saveProfileAs(table);
    }

    //TODO: в MainWindow
    //statusBar()->showMessage("Файл успешно сохранен", 5000);
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

