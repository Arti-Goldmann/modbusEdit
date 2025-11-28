#include "outfilegenerator.h"

OutFileGenerator::OutFileGenerator(QWidget* parent) : parent(parent) {}

bool OutFileGenerator::setGenerationPath() {

    QString lastDir = getLastDirectory();
    QString fileGenDir = QFileDialog::getExistingDirectory(
        parent,
        QObject::tr("Выберите директорию"),
        lastDir,
        QFileDialog::ShowDirsOnly
        );

    if (fileGenDir.isEmpty()) {
        setError("Директория генерации не найдена");
        return false;
    }

    // Нашли директорию
    currentGenFilePath = fileGenDir + "/MBedit.c";
    saveLastDirectory(currentGenFilePath);

    return true;
}

bool OutFileGenerator::generate(const QJsonArray& data, const QJsonArray& baseValues) {
    lastError.clear();

    QFile file(currentGenFilePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        setError(QString("Не удалось открыть файл: %1\nОшибка: %2").arg(currentGenFilePath, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    try{
        //TODO: coils и DI тоже сделать
        out << "//The file was generated automatically.\n#include \"MBedit.h\"\n\n";
        //Формируем функцию на чтение для RW (Holding Registers RW)
        out << funcHandlerGen("MBhandlerHR_R", {Constants::AccessType::READ_WRITE, Constants::AccessType::READ_WRITE_IN_STOP}, data, baseValues, FOR_READ);
        //Формируем функцию на запись для RW (Holding Registers RW)
        out << funcHandlerGen("MBhandlerHR_W", {Constants::AccessType::READ_WRITE, Constants::AccessType::READ_WRITE_IN_STOP}, data, baseValues, FOR_WRITE);
        //Формируем функцию на чтение для R (Input Registers R)
        out << funcHandlerGen("MBhandlerIR_R", {Constants::AccessType::READ_ONLY}, data, baseValues, FOR_READ);

        out << "// R/W-variables.\n";
        out << arrayGen("mbodHR", {Constants::AccessType::READ_WRITE, Constants::AccessType::READ_WRITE_IN_STOP}, data);
        out << arrayGen("mbodIR", {Constants::AccessType::READ_ONLY}, data);

        //TODO: coils и DI тоже сделать
        out << "TModbusSlaveDictObj mbodC[] =\n";
        out << "{\n";
        out << "    0, 0xFFFF   // конец\n";
        out << "};\n";
        out << "\n";
        out << "TModbusSlaveDictObj mbodDI[] =\n";
        out << "{\n";
        out << "    0, 0xFFFF   // конец\n";
        out << "};\n";
        out << "\n";

        file.close();
    } catch (const std::exception& e) {
        setError(QString("Ошибка при генерации: %1").arg(e.what()));
        file.close();
        return false;
    }
    return true;
}

QString OutFileGenerator::funcHandlerGen(const QString& funcName, const QStringList& targetAccessTypes, const QJsonArray& data, const QJsonArray& baseValues, TdirectionType readOrWrite) {
    QString output;

    //Начало функции
    output.append("void " + funcName + "(TModbusSlaveDictObj* reg)\n{\n\tswitch (reg->mbIndex)\n\t{\n");

    for(const QJsonValue &val : std::as_const(data)) {

        QJsonObject obj = val.toObject();
        QString accessType = obj[Constants::JsonKeys::Data::ACCESS_TYPE].toString();

        if(targetAccessTypes.contains(accessType)) { //Нашли объект с нужным типом доступа
            QString baseValue;
            QString IQformat;

            //Ищем base значение для этой data
            for(const QJsonValue &baseVal : std::as_const(baseValues)) {
                QJsonObject baseObj = baseVal.toObject();

                if(obj[Constants::JsonKeys::Data::BASE] == baseObj[Constants::JsonKeys::BaseValues::BASE_NAME]) { //Имена базовой величины совпали
                    baseValue = baseObj[Constants::JsonKeys::BaseValues::BASE_VALUE].toString();
                    IQformat = baseObj[Constants::JsonKeys::BaseValues::IQ_FORMAT].toString();
                }
            }

            //Начало switch case
            output.append(QString("\t\tcase %1:\n").arg(obj[Constants::JsonKeys::Data::ADDRESS_DEC].toString().toInt())); //Пишем case и адрес регистра
            output.append("\t\t{\n");

            //Если только в стопе, и формируем код на запись, то добавляем проверку состояние в стопе ли СУ
            if(accessType == Constants::AccessType::READ_WRITE_IN_STOP && readOrWrite == FOR_WRITE) {
                output.append(QString("\t\tif(!%1){\n").arg(IS_DRV_IN_STOP)); //Открыли if(...) {
            }

            QString switchCaseCode = "";
            if(obj[Constants::JsonKeys::Data::PARAM_TYPE] == Constants::ParamType::COMMON) {
                //Формируем функцию на чтение или запись
                switchCaseCode = (readOrWrite == FOR_READ) ? funcHandlerGen_R(funcName, obj, IQformat, baseValue):
                                                             funcHandlerGen_W(funcName, obj, IQformat, baseValue);
            } else if(obj[Constants::JsonKeys::Data::PARAM_TYPE] == Constants::ParamType::USER) {
                switchCaseCode = (readOrWrite == FOR_READ) ? "\t\t\t" + obj[Constants::JsonKeys::Data::USER_CODE_R].toString() + "\n":
                                                             "\t\t\t" + obj[Constants::JsonKeys::Data::USER_CODE_W].toString() + "\n";
            }

            output.append(switchCaseCode);

            if(accessType == Constants::AccessType::READ_WRITE_IN_STOP && readOrWrite == FOR_WRITE) {
                output.append(QString("\t\t}\n")); //Закрыли if(...) {
            }

            //Конец switch case
            output.append("\t\t\tbreak;\n");
            output.append("\t\t}\n");
        }
    }

    //Конец
    output.append("\t\tdefault:\n\t\t\tbreak;\n\t}\n}\n\n");
    return output;
}

QString OutFileGenerator::funcHandlerGen_R(const QString& funcName, const QJsonObject& obj, const QString& IQformat, const QString& baseValue) {
    QString output;
    QString drvDataType = obj[Constants::JsonKeys::Data::DRV_DATA_TYPE].toString();
    QString varName = obj[Constants::JsonKeys::Data::VAR_NAME].toString();
    QString modbusDataType = obj[Constants::JsonKeys::Data::MODBUS_DATA_TYPE].toString();

    if (Constants::drvDataType::isStandartNum(drvDataType) || drvDataType == Constants::drvDataType::IQ0) { //IQ0 обрабатываем как обычный StandartNum
        // Для StandartNum (целочисленных типов) - прямая передача без преобразования
        output.append(QString("\t\t\treg->data = %1;\n").arg(varName));
    } else if (Constants::drvDataType::isIQ(drvDataType) && drvDataType != Constants::drvDataType::IQ0) {
        // Для IQ типов - используем функцию преобразования IQ
        QString iqNumber = IQformatToIQNumber(drvDataType);  // Извлекаем номер из "IQ24" -> "24"
        // Формируем имя функции: "MBedit_IQ24toInt16" или "MBedit_IQ24toUint16"
        QString funcName = QString("MBedit_IQ%1to%2").arg(iqNumber, modbusDataType);
        output.append(QString("\t\t\treg->data = %1(%2, %3, %4, %5);\n")
                          .arg(funcName,
                               varName,
                               obj[Constants::JsonKeys::Data::GAIN].toString(),
                               baseValue,
                               IQformatToBaseQ(IQformat)
                               )
                      );
    } else if (Constants::drvDataType::isFloat(drvDataType)) {
        // Для float - используем функцию преобразования float
        // Формируем имя функции: "MBedit_FloattoInt16" или "MBedit_FloattoUint16"
        QString funcName = QString("MBedit_FloatTo%1").arg(modbusDataType);
        output.append(QString("\t\t\treg->data = %1(%2, %3);\n")
                          .arg(funcName,
                               varName,
                               obj[Constants::JsonKeys::Data::GAIN].toString()
                               )
                      );
    }

    return output;
}

QString OutFileGenerator::funcHandlerGen_W(const QString& funcName, const QJsonObject& obj, const QString& IQformat, const QString& baseValue) {
    QString output;
    QString drvDataType = obj[Constants::JsonKeys::Data::DRV_DATA_TYPE].toString();
    QString varName = obj[Constants::JsonKeys::Data::VAR_NAME].toString();
    QString minValue = obj[Constants::JsonKeys::Data::MIN].toString();
    QString maxValue = obj[Constants::JsonKeys::Data::MAX].toString();
    QString modbusDataType = obj[Constants::JsonKeys::Data::MODBUS_DATA_TYPE].toString();

    if (Constants::drvDataType::isStandartNum(drvDataType)|| drvDataType == Constants::drvDataType::IQ0) { //IQ0 обрабатываем как обычный StandartNum
        // Для StandartNum (целочисленных типов)
        // Определяем тип данных в зависимости от signed/unsigned
        bool isSigned = Constants::drvDataType::isSigned(drvDataType) || drvDataType == Constants::drvDataType::IQ0;
        QString dataType = isSigned ? "int16" : "Uint16";

        output.append(QString("\t\t\t%1 data = reg->data;\n").arg(dataType));

        // Добавляем ограничения по min и max через if
        if (!minValue.isEmpty()) {
            output.append(QString("\t\t\tif (data < (%1)%2) data = (%1)%2;\n").arg(dataType, minValue));
        }
        if (!maxValue.isEmpty()) {
            output.append(QString("\t\t\tif (data > (%1)%2) data = (%1)%2;\n").arg(dataType, maxValue));
        }

        output.append(QString("\t\t\tMBedit_WriteToValue((void*)&%1, data, sizeof(%1));\n").arg(varName));

    } else if (Constants::drvDataType::isIQ(drvDataType) && drvDataType != Constants::drvDataType::IQ0) {
        // Для IQ типов
        QString iqNumber = IQformatToIQNumber(drvDataType);  // Извлекаем номер из "IQ24" -> "24"
        // Формируем имя функции: "MBedit_Int16toIQ24" или "MBedit_Uint16toIQ24"
        QString funcName = QString("MBedit_%1toIQ%2").arg(modbusDataType, iqNumber);

        QString min = minValue.isEmpty() ? "-1e38f" : minValue;
        QString max = maxValue.isEmpty() ? "1e38f" : maxValue;

        output.append(QString("\t\t\tint32 data = %1(reg->data, %2, %3, %4, %5, %6);\n")
                          .arg(funcName,
                               obj[Constants::JsonKeys::Data::GAIN].toString(),
                               baseValue,
                               IQformatToBaseQ(IQformat),
                               max,
                               min
                               )
                      );
        output.append(QString("\t\t\tMBedit_WriteToValue((void*)&%1, data, sizeof(%1));\n").arg(varName));

    } else if (Constants::drvDataType::isFloat(drvDataType)) {
        // Для float
        // Формируем имя функции: "MBedit_Int16toFloat" или "MBedit_Uint16toFloat"
        QString funcName = QString("MBedit_%1toFloat").arg(modbusDataType);

        QString min = minValue.isEmpty() ? "-1e38f" : minValue;
        QString max = maxValue.isEmpty() ? "1e38f" : maxValue;

        output.append(QString("\t\t\tfloat data = %1(reg->data, %2, %3, %4);\n")
                          .arg(funcName,
                               obj[Constants::JsonKeys::Data::GAIN].toString(),
                               max,
                               min
                               )
                      );
        output.append(QString("\t\t\tMBedit_WriteToValue((void*)&%1, data, sizeof(%1));\n").arg(varName));
    }

    return output;
}

QString OutFileGenerator::arrayGen(const QString& arrName, const QStringList& targetAccessTypes, const QJsonArray& data) {
    QString output;

    //Начало массива
    output.append("TModbusSlaveDictObj " + arrName + "[]=\n\{\n");

    for(const QJsonValue &val : std::as_const(data)) {

        QJsonObject obj = val.toObject();
        QString accessType = obj[Constants::JsonKeys::Data::ACCESS_TYPE].toString();

        if(targetAccessTypes.contains(accessType)) { //Нашли объект с нужным типом
            QString comment = obj[Constants::JsonKeys::Data::PARAM_TYPE] == Constants::ParamType::COMMON ? obj[Constants::JsonKeys::Data::VAR_NAME].toString() : Constants::UiText::USER_CODE;
            QString addressDec = obj[Constants::JsonKeys::Data::ADDRESS_DEC].toString();
            output.append(QString("\t%1, 0,   //%2\n").arg(addressDec, comment));
        }
    }

    //Конец
    output.append("\t0, 0xFFFF   //end\n};\n\n");
    return output;
}

void OutFileGenerator::setError(const QString& message) {
    lastError = message;
}

QString OutFileGenerator::getLastError() {
    return lastError;
}

QString OutFileGenerator::getLastDirectory() const {
    QSettings settings("MPEI", "modbusEdit");
    QString lastDir = settings.value("lastDirFileGen", QDir::homePath()).toString();
    return lastDir;
}

void OutFileGenerator::saveLastDirectory(const QString& path) {
    QSettings settings("MPEI", "modbusEdit");
    QFileInfo fileInfo(path);
    settings.setValue("lastDirFileGen", fileInfo.absolutePath());
}

QString OutFileGenerator::IQformatToBaseQ(const QString& str) {
    int dotIndex = str.indexOf('.');
    if (dotIndex == -1) {
        return "0";  // Нет точки - дробная часть = 0
    }
    return str.mid(dotIndex + 1);  // Всё после точки
}

QString OutFileGenerator::IQformatToIQNumber(const QString& str) {
    // Извлекаем номер из строки типа "IQ24" -> "24"
    if (str.startsWith("IQ")) {
        return str.mid(2);  // Всё после "IQ"
    }
    return str;
}

QString OutFileGenerator::restoreLastGenFilePath() {
    QString fileGenDir = getLastDirectory();
    currentGenFilePath = fileGenDir + "/MBedit.c";
    return currentGenFilePath;
}
