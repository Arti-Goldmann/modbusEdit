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

bool OutFileGenerator::generate(const QJsonArray& jsonArr, const QString& absGenPath) {
    lastError.clear();

    QFile file(absGenPath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text)){
        setError(QString("Не удалось открыть файл: %1\nОшибка: %2").arg(absGenPath, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    try{
        //TODO: coils и DI тоже сделать
        //Формируем функцию на чтение для RW (Holding Registers RW)
        out << funcHandlerGen("MBhandlerHR_R", "RW", jsonArr, FOR_READ);
        //Формируем функцию на запись для RW (Holding Registers RW)
        out << funcHandlerGen("MBhandlerHR_W", "RW", jsonArr, FOR_WRITE);
        //Формируем функцию на чтение для R (Input Registers R)
        out << funcHandlerGen("MBhandlerIR_R", "R", jsonArr, FOR_READ);

        out << "// R/W-переменные.\n";
        out << arrayGen("mbodHR", "RW", jsonArr);
        out << arrayGen("mbodIR", "R", jsonArr);

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

QString OutFileGenerator::funcHandlerGen(const QString& funcName, const QString&type, const QJsonArray& jsonArr, TdirectionType readOrWrite) {
    QString output;

    //Начало функции
    output.append("void " + funcName + "(TModbusSlaveDictObj* reg)\n{\n\tswitch (reg->mbIndex)\n\t{\n");

    for(const QJsonValue &val : std::as_const(jsonArr)) {

        QJsonObject obj = val.toObject();

        if(obj["accessType"] == type) { //Нашли объект с нужным типом доступа
            //Начало switch case
            output.append(QString("\t\tcase %1:\n").arg(obj["addressDec"].toString().toInt())); //Пишем case и адрес регистра

            //Формируем функцию на чтение или запись
            QString switchCaseCode = (readOrWrite == FOR_READ) ? funcHandlerGen_R(funcName, obj):
                                                                 funcHandlerGen_W(funcName, obj);

            output.append(switchCaseCode);

            //Конец switch case
            output.append("\t\t\tbreak;\n");
        }
    }

    //Конец
    output.append("\t\tdefault:\n\t\t\tbreak;\n\t}\n}\n\n");
    return output;
}

QString OutFileGenerator::funcHandlerGen_R(const QString& funcName, const QJsonObject& obj) {
    QString output;

    //Формируем функцию для этого case в соотвествии с полями json
    output.append(QString("\t\t\treg->data = %1(%2,%3,%4,%5);\n")
                      .arg(IQ_TO_TYPE_FUNC[obj["dataType"].toString()],
                           "varName", //TODO: varName
                           obj["gain"].toString(),
                           "base", //TODO: base
                           "0" //TODO: Qbase
                           )
                  );

    return output;
}

QString OutFileGenerator::funcHandlerGen_W(const QString& funcName, const QJsonObject& obj) {
    QString output;

    //Формируем функцию для этого case в соотвествии с полями json
    output.append(QString("\t\t\t%2 = %1(reg->data,%3,%4,%5);\n")
                      .arg(TYPE_TO_IQ_FUNC[obj["dataType"].toString()],
                           "varName", //TODO: varName
                           obj["gain"].toString(),
                           "base", //TODO: base
                           "0" //TODO: Qbase
                           )
                  );

    return output;
}

QString OutFileGenerator::arrayGen(const QString& arrName, const QString&type, const QJsonArray& jsonArr) {
    QString output;

    //Начало массива
    output.append("TModbusSlaveDictObj*" + arrName + "[]=\n\{\n");

    for(const QJsonValue &val : std::as_const(jsonArr)) {

        QJsonObject obj = val.toObject();

        if(obj["accessType"] == type) { //Нашли объект с нужным типом
            output.append("\t"+ QString(obj["addressDec"].toString()) + ", 0,   //varName\n"); //адрес регистра, 0, varName //TODO: название переменной тоже писать
        }
    }

    //Конец
    output.append("\t0, 0xFFFF   //конец\n};\n\n");
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
