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
        out << funcHandlerGen("MBhandlerHR_R", Constants::AccessType::READ_WRITE, data, baseValues, FOR_READ);
        //Формируем функцию на запись для RW (Holding Registers RW)
        out << funcHandlerGen("MBhandlerHR_W", Constants::AccessType::READ_WRITE, data, baseValues, FOR_WRITE);
        //Формируем функцию на чтение для R (Input Registers R)
        out << funcHandlerGen("MBhandlerIR_R", Constants::AccessType::READ_ONLY, data, baseValues, FOR_READ);

        out << "// R/W-variables.\n";
        out << arrayGen("mbodHR", Constants::AccessType::READ_WRITE, data);
        out << arrayGen("mbodIR", Constants::AccessType::READ_ONLY, data);

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

QString OutFileGenerator::funcHandlerGen(const QString& funcName, const QString& targetType, const QJsonArray& data, const QJsonArray& baseValues, TdirectionType readOrWrite) {
    QString output;

    //Начало функции
    output.append("void " + funcName + "(TModbusSlaveDictObj* reg)\n{\n\tswitch (reg->mbIndex)\n\t{\n");

    for(const QJsonValue &val : std::as_const(data)) {

        QJsonObject obj = val.toObject();

        if(obj[Constants::JsonKeys::Data::ACCESS_TYPE] == targetType) { //Нашли объект с нужным типом доступа
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

            //Конец switch case
            output.append("\t\t\tbreak;\n");
        }
    }

    //Конец
    output.append("\t\tdefault:\n\t\t\tbreak;\n\t}\n}\n\n");
    return output;
}

QString OutFileGenerator::funcHandlerGen_R(const QString& funcName, const QJsonObject& obj, const QString& IQformat, const QString& baseValue) {
    QString output;

    //Формируем функцию для этого case в соотвествии с полями json
    output.append(QString("\t\t\treg->data = %1(%2,%3,%4,%5);\n")
                      .arg(IQ_TO_TYPE_FUNC[obj[Constants::JsonKeys::Data::DATA_TYPE].toString()],
                           obj[Constants::JsonKeys::Data::VAR_NAME].toString(),
                           obj[Constants::JsonKeys::Data::GAIN].toString(),
                           baseValue,
                           IQformatToBaseQ(IQformat)
                           )
                  );

    return output;
}

QString OutFileGenerator::funcHandlerGen_W(const QString& funcName, const QJsonObject& obj, const QString& IQformat, const QString& baseValue) {
    QString output;

    //Формируем функцию для этого case в соотвествии с полями json
    output.append(QString("\t\t\t%2 = %1(reg->data,%3,%4,%5);\n")
                      .arg(TYPE_TO_IQ_FUNC[obj[Constants::JsonKeys::Data::DATA_TYPE].toString()],
                           obj[Constants::JsonKeys::Data::VAR_NAME].toString(),
                           obj[Constants::JsonKeys::Data::GAIN].toString(),
                           baseValue,
                           IQformatToBaseQ(IQformat)
                           )
                  );

    return output;
}

QString OutFileGenerator::arrayGen(const QString& arrName, const QString& targetType, const QJsonArray& data) {
    QString output;

    //Начало массива
    output.append("TModbusSlaveDictObj*" + arrName + "[]=\n\{\n");

    for(const QJsonValue &val : std::as_const(data)) {

        QJsonObject obj = val.toObject();
        if(obj[Constants::JsonKeys::Data::ACCESS_TYPE] == targetType) { //Нашли объект с нужным типом
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

QString OutFileGenerator::restoreLastGenFilePath() {
    QString fileGenDir = getLastDirectory();
    currentGenFilePath = fileGenDir + "/MBedit.c";
    return currentGenFilePath;
}
