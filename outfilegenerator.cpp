#include "outfilegenerator.h"

OutFileGenerator::OutFileGenerator() {}

bool OutFileGenerator::generate() {

    //TODO: временно открываем профиль по захардкоженному пути
    // Открытие файла
    QFile profileFile{"D:/DATA/Documents/GitRep/MyRep/software/PC/modbusEdit/profile.json"};
    if(!profileFile.open(QIODevice::ReadOnly)) {
        showProfileError(QString("Не удалось открыть файл: %1\nОшибка: %2")
                             .arg("D:/DATA/Documents/GitRep/MyRep/software/PC/modbusEdit/profile.json", profileFile.errorString()), "Ошибка загрузки профиля");
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

    QJsonArray jsonArr = doc.array();


    QFile file("D:/DATA/Documents/GitRep/MyRep/software/PC/modbusEdit/outFile.c"); //TODO: временно беру захардкоженный путь
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

     //TODO: coils и DI тоже сделать
    //Формируем функцию на чтение для RW (Holding Registers RW)
    out << funcHandlerGen("MBhandlerHR_R", "RW", jsonArr, IQ_TO_TYPE_FUNC);
    //Формируем функцию на запись для RW (Holding Registers RW)
    out << funcHandlerGen("MBhandlerHR_W", "RW", jsonArr, TYPE_TO_IQ_FUNC);
    //Формируем функцию на чтение для R (Input Registers R)
    out << funcHandlerGen("MBhandlerIR_R", "R", jsonArr, IQ_TO_TYPE_FUNC);

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
    return true;
}

void OutFileGenerator::showProfileError(const QString& message, const QString& title) {
    qCritical() << message;
}

QString OutFileGenerator::funcHandlerGen(const QString& funcName, const QString&type, const QJsonArray& jsonArr, const QHash<QString, QString>& FUNC) {
    QString output;

    //Начало функции
    output.append("void " + funcName + "(TModbusSlaveDictObj* reg)\n{\n\tswitch (reg->mbIndex)\n\t{\n");

    for(const QJsonValue &val : std::as_const(jsonArr)) {

        QJsonObject obj = val.toObject();

        if(obj["accessType"] == type) { //Нашли объект с нужным типом
            output.append(QString("\t\tcase %1:\n").arg(obj["addressDec"].toString().toInt())); //Пишем case и адрес регистра

            //Формируем функцию для этого case в соотвествии с полями json
            output.append(QString("\t\t\treg->data = %1(%2,%3,%4,%5);\n")
                              .arg(FUNC[obj["dataType"].toString()],
                                   "varName", //TODO: varName
                                   obj["range"].toString(),
                                   "base", //TODO: base
                                   "0" //TODO: Qbase
                                   )
                          );

            output.append("\t\t\tbreak;\n");
        }
    }

    //Конец
    output.append("\t\tdefault:\n\t\t\tbreak;\n\t}\n}\n\n");
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
