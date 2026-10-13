#include "excelexchange.h"
#include "constants.h"
#include "addressconverter.h"

#include <QBuffer>
#include <QFile>
#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <cmath>

#include <private/qzipreader_p.h>
#include <private/qzipwriter_p.h>

namespace {

using namespace Constants::JsonKeys;

// Значения столбца "Доступ" в Excel <-> тип доступа в программе
const QVector<QPair<QString, QString>>& accessMap() {
    static const QVector<QPair<QString, QString>> map = {
        {Constants::AccessType::READ_ONLY,            QString::fromUtf8("Чтение (04)")},
        {Constants::AccessType::READ_WRITE,           QString::fromUtf8("Чтение/запись (03/06/10)")},
        {Constants::AccessType::READ_WRITE_IN_STOP,   QString::fromUtf8("Чтение/запись в останове (03/06/10)")},
        {QString::fromUtf8(Constants::AccessType::READ_WRITE_READ_ONLY), QString::fromUtf8("Чтение (03)")},
    };
    return map;
}

QString accessToExcel(const QString& access) {
    for (const auto& pair : accessMap()) {
        if (pair.first == access) return pair.second;
    }
    return access;
}

QString accessFromExcel(const QString& text) {
    const QString value = text.trimmed();
    for (const auto& pair : accessMap()) {
        if (pair.second.compare(value, Qt::CaseInsensitive) == 0 ||
            pair.first.compare(value, Qt::CaseInsensitive) == 0) {
            return pair.first;
        }
    }
    return value; // Неизвестное значение оставляем как есть
}

// Число в текст без хвостов вида 0.10000000000000001
QString formatNumber(double value) {
    if (std::abs(value - std::round(value)) < 1e-9 && std::abs(value) < 1e15) {
        return QString::number(static_cast<qint64>(std::llround(value)));
    }
    return QString::number(value, 'g', 10);
}

// Коэффициент в профиле всегда с точкой, чтобы в C-коде он был вещественным
QString formatGain(double value) {
    QString text = formatNumber(value);
    if (!text.contains('.') && !text.contains('e', Qt::CaseInsensitive)) {
        text += ".0";
    }
    return text;
}

double parseNumber(const QString& text, bool* ok) {
    QString normalized = text.trimmed();
    normalized.replace(',', '.');
    return normalized.toDouble(ok);
}

// "AB" -> 27 (с единицы)
int columnFromLetters(const QString& ref) {
    int col = 0;
    for (const QChar ch : ref) {
        if (!ch.isLetter()) break;
        col = col * 26 + (ch.toUpper().unicode() - 'A' + 1);
    }
    return col;
}

int rowFromRef(const QString& ref) {
    int pos = 0;
    while (pos < ref.size() && ref[pos].isLetter()) pos++;
    return ref.mid(pos).toInt();
}

QString columnLetters(int col) { // col с единицы
    QString letters;
    while (col > 0) {
        int rem = (col - 1) % 26;
        letters.prepend(QChar('A' + rem));
        col = (col - 1) / 26;
    }
    return letters;
}

QString headerKey(const QString& text) {
    return text.simplified().toLower();
}

// ---------------------------------------------------------------------------
// Запись xlsx

enum Style {
    STYLE_DEFAULT = 0,
    STYLE_HEADER = 1,   // Шапка: жирный, серый фон, по центру
    STYLE_TITLE = 2,    // Заголовок группы: жирный, светло-серый фон
    STYLE_CENTER = 3,   // Данные по центру
    STYLE_LEFT = 4,     // Данные по левому краю
};

const char* XML_HEADER = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

QByteArray stylesXml() {
    return QByteArray(XML_HEADER) +
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<fonts count=\"3\">"
        "<font><sz val=\"11\"/><name val=\"Calibri\"/><family val=\"2\"/></font>"
        "<font><sz val=\"11\"/><name val=\"Times New Roman\"/><family val=\"1\"/></font>"
        "<font><b/><sz val=\"11\"/><name val=\"Times New Roman\"/><family val=\"1\"/></font>"
        "</fonts>"
        "<fills count=\"4\">"
        "<fill><patternFill patternType=\"none\"/></fill>"
        "<fill><patternFill patternType=\"gray125\"/></fill>"
        "<fill><patternFill patternType=\"solid\"><fgColor rgb=\"FFD9D9D9\"/><bgColor indexed=\"64\"/></patternFill></fill>"
        "<fill><patternFill patternType=\"solid\"><fgColor rgb=\"FFF2F2F2\"/><bgColor indexed=\"64\"/></patternFill></fill>"
        "</fills>"
        "<borders count=\"2\">"
        "<border><left/><right/><top/><bottom/><diagonal/></border>"
        "<border>"
        "<left style=\"thin\"><color indexed=\"64\"/></left>"
        "<right style=\"thin\"><color indexed=\"64\"/></right>"
        "<top style=\"thin\"><color indexed=\"64\"/></top>"
        "<bottom style=\"thin\"><color indexed=\"64\"/></bottom>"
        "<diagonal/></border>"
        "</borders>"
        "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
        "<cellXfs count=\"5\">"
        "<xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/>"
        "<xf numFmtId=\"0\" fontId=\"2\" fillId=\"2\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyFill=\"1\" applyBorder=\"1\" applyAlignment=\"1\">"
        "<alignment horizontal=\"center\" vertical=\"center\" wrapText=\"1\"/></xf>"
        "<xf numFmtId=\"0\" fontId=\"2\" fillId=\"3\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyFill=\"1\" applyBorder=\"1\" applyAlignment=\"1\">"
        "<alignment horizontal=\"left\" vertical=\"center\"/></xf>"
        "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyBorder=\"1\" applyAlignment=\"1\">"
        "<alignment horizontal=\"center\" vertical=\"top\" wrapText=\"1\"/></xf>"
        "<xf numFmtId=\"0\" fontId=\"1\" fillId=\"0\" borderId=\"1\" xfId=\"0\" applyFont=\"1\" applyBorder=\"1\" applyAlignment=\"1\">"
        "<alignment horizontal=\"left\" vertical=\"top\" wrapText=\"1\"/></xf>"
        "</cellXfs>"
        "<cellStyles count=\"1\"><cellStyle name=\"Normal\" xfId=\"0\" builtinId=\"0\"/></cellStyles>"
        "</styleSheet>";
}

// Ячейка листа для записи
struct OutCell {
    QString text;
    bool isNumber = false;
    int style = STYLE_DEFAULT;
};

// Строка листа: либо обычные ячейки, либо заголовок группы на всю ширину
struct OutRow {
    QVector<OutCell> cells;
    bool merged = false;
};

// Удаляем символы, недопустимые в XML 1.0 (могут попасться в пользовательском коде)
QString xmlSafe(const QString& text) {
    QString result;
    result.reserve(text.size());
    for (const QChar ch : text) {
        const ushort code = ch.unicode();
        if (code >= 0x20 || code == '\t' || code == '\n' || code == '\r') {
            result.append(ch);
        }
    }
    return result;
}

QByteArray sheetXml(const QVector<ExcelExchange::Column>& columns, const QVector<OutRow>& rows) {
    QByteArray out;
    QXmlStreamWriter xml(&out);
    xml.writeStartDocument("1.0", true);
    xml.writeStartElement("worksheet");
    xml.writeDefaultNamespace("http://schemas.openxmlformats.org/spreadsheetml/2006/main");

    const int colCount = columns.size();
    const QString lastCol = columnLetters(colCount);
    const int rowCount = rows.size() + 1;

    xml.writeEmptyElement("dimension");
    xml.writeAttribute("ref", QString("A1:%1%2").arg(lastCol).arg(rowCount));

    // Закрепляем шапку
    xml.writeStartElement("sheetViews");
    xml.writeStartElement("sheetView");
    xml.writeAttribute("workbookViewId", "0");
    xml.writeEmptyElement("pane");
    xml.writeAttribute("ySplit", "1");
    xml.writeAttribute("topLeftCell", "A2");
    xml.writeAttribute("activePane", "bottomLeft");
    xml.writeAttribute("state", "frozen");
    xml.writeEndElement(); // sheetView
    xml.writeEndElement(); // sheetViews

    xml.writeStartElement("cols");
    for (int i = 0; i < colCount; i++) {
        xml.writeEmptyElement("col");
        xml.writeAttribute("min", QString::number(i + 1));
        xml.writeAttribute("max", QString::number(i + 1));
        xml.writeAttribute("width", QString::number(columns[i].width));
        xml.writeAttribute("customWidth", "1");
    }
    xml.writeEndElement(); // cols

    auto writeCell = [&xml](const QString& ref, const OutCell& cell) {
        xml.writeStartElement("c");
        xml.writeAttribute("r", ref);
        xml.writeAttribute("s", QString::number(cell.style));
        if (cell.text.isEmpty()) {
            // Пустая ячейка - только стиль (рамка)
        } else if (cell.isNumber) {
            xml.writeTextElement("v", cell.text);
        } else {
            xml.writeAttribute("t", "inlineStr");
            xml.writeStartElement("is");
            xml.writeStartElement("t");
            xml.writeAttribute("xml:space", "preserve");
            xml.writeCharacters(xmlSafe(cell.text));
            xml.writeEndElement(); // t
            xml.writeEndElement(); // is
        }
        xml.writeEndElement(); // c
    };

    QStringList mergeRefs;

    xml.writeStartElement("sheetData");

    // Шапка
    xml.writeStartElement("row");
    xml.writeAttribute("r", "1");
    for (int i = 0; i < colCount; i++) {
        OutCell cell;
        cell.text = columns[i].header;
        cell.style = STYLE_HEADER;
        writeCell(columnLetters(i + 1) + "1", cell);
    }
    xml.writeEndElement();

    for (int r = 0; r < rows.size(); r++) {
        const int excelRow = r + 2;
        xml.writeStartElement("row");
        xml.writeAttribute("r", QString::number(excelRow));
        for (int i = 0; i < colCount; i++) {
            OutCell cell = (i < rows[r].cells.size()) ? rows[r].cells[i] : OutCell();
            if (rows[r].merged) {
                cell.style = STYLE_TITLE; // Рамка и фон у всех ячеек объединения
            }
            writeCell(columnLetters(i + 1) + QString::number(excelRow), cell);
        }
        xml.writeEndElement(); // row

        if (rows[r].merged && colCount > 1) {
            mergeRefs << QString("A%1:%2%1").arg(QString::number(excelRow), lastCol);
        }
    }
    xml.writeEndElement(); // sheetData

    xml.writeEmptyElement("autoFilter");
    xml.writeAttribute("ref", QString("A1:%1%2").arg(lastCol).arg(rowCount));

    if (!mergeRefs.isEmpty()) {
        xml.writeStartElement("mergeCells");
        xml.writeAttribute("count", QString::number(mergeRefs.size()));
        for (const QString& ref : std::as_const(mergeRefs)) {
            xml.writeEmptyElement("mergeCell");
            xml.writeAttribute("ref", ref);
        }
        xml.writeEndElement();
    }

    xml.writeEndElement(); // worksheet
    xml.writeEndDocument();
    return out;
}

QVector<ExcelExchange::Column> pickColumns(const QVector<ExcelExchange::Column>& all, const QStringList& ids) {
    QVector<ExcelExchange::Column> result;
    for (const auto& column : all) {
        if (ids.contains(column.id)) result << column;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Чтение xlsx

struct InSheet {
    QMap<int, QMap<int, QString>> cells; // строка -> (столбец -> текст), с единицы
    QHash<int, int> mergedFromFirstCol;  // строка -> ширина объединения, начинающегося в столбце A
};

QStringList readSharedStrings(const QByteArray& data) {
    QStringList strings;
    QXmlStreamReader xml(data);
    QString current;
    bool inSi = false;
    int phoneticDepth = 0; // <rPh> - фонетическая подсказка, ее текст не берем

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement()) {
            if (xml.name() == u"si") {
                inSi = true;
                current.clear();
            } else if (xml.name() == u"rPh") {
                phoneticDepth++;
            } else if (xml.name() == u"t" && inSi && phoneticDepth == 0) {
                current += xml.readElementText();
            }
        } else if (xml.isEndElement()) {
            if (xml.name() == u"si") {
                strings << current;
                inSi = false;
            } else if (xml.name() == u"rPh") {
                phoneticDepth--;
            }
        }
    }
    return strings;
}

InSheet readSheet(const QByteArray& data, const QStringList& sharedStrings) {
    InSheet sheet;
    QXmlStreamReader xml(data);
    int currentRow = 0;
    int currentCol = 0;

    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement()) continue;

        if (xml.name() == u"row") {
            const QString r = xml.attributes().value("r").toString();
            currentRow = r.isEmpty() ? currentRow + 1 : r.toInt();
            currentCol = 0;
        } else if (xml.name() == u"c") {
            const QString ref = xml.attributes().value("r").toString();
            const QString type = xml.attributes().value("t").toString();
            currentCol = ref.isEmpty() ? currentCol + 1 : columnFromLetters(ref);

            QString value;
            QString inlineText;
            // Читаем содержимое ячейки до закрывающего </c>
            while (!(xml.isEndElement() && xml.name() == u"c") && !xml.atEnd()) {
                xml.readNext();
                if (xml.isStartElement()) {
                    if (xml.name() == u"v") {
                        value = xml.readElementText();
                    } else if (xml.name() == u"t") {
                        inlineText += xml.readElementText();
                    }
                }
            }

            QString text;
            if (type == "s") {
                text = sharedStrings.value(value.toInt());
            } else if (type == "inlineStr") {
                text = inlineText;
            } else if (type == "str" || type == "e" || type == "b") {
                text = value;
            } else if (!value.isEmpty()) {
                bool ok;
                double number = value.toDouble(&ok);
                text = ok ? formatNumber(number) : value;
            }

            if (!text.trimmed().isEmpty()) {
                sheet.cells[currentRow][currentCol] = text;
            }
        } else if (xml.name() == u"mergeCell") {
            const QStringList parts = xml.attributes().value("ref").toString().split(':');
            if (parts.size() == 2 && columnFromLetters(parts[0]) == 1) {
                sheet.mergedFromFirstCol[rowFromRef(parts[0])] = columnFromLetters(parts[1]);
            }
        }
    }
    return sheet;
}

// Сопоставление номеров столбцов файла с id столбцов по тексту шапки
QHash<QString, int> mapHeader(const InSheet& sheet, const QVector<ExcelExchange::Column>& columns) {
    QHash<QString, int> result;
    if (sheet.cells.isEmpty()) return result;

    const auto& header = sheet.cells.first();
    for (auto it = header.begin(); it != header.end(); ++it) {
        const QString key = headerKey(it.value());
        for (const auto& column : columns) {
            bool match = headerKey(column.header) == key;
            for (const QString& alias : column.aliases) {
                match = match || headerKey(alias) == key;
            }
            if (match && !result.contains(column.id)) {
                result[column.id] = it.key();
            }
        }
    }
    return result;
}

} // namespace

// ---------------------------------------------------------------------------

const QVector<ExcelExchange::Column>& ExcelExchange::dataColumns() {
    using namespace Constants::TableHeaders;
    static const QVector<Column> columns = {
        {Data::ADDRESS_HEX,      QString::fromUtf8("Адрес"),                     9,  true,  true,  {ADDRESS_HEX, "Адрес (hex)"}},
        {Data::ADDRESS_DEC,      QString::fromUtf8("Адрес, дес."),               12, true,  true,  {ADDRESS_DEC}},
        {Data::GROUP_NAME,       QString::fromUtf8("Наименование"),              34, false, true,  {GROUP_NAME}},
        {Data::ACCESS_TYPE,      QString::fromUtf8("Доступ"),                    22, true,  true,  {ACCESS_TYPE}},
        {Data::MODBUS_DATA_TYPE, QString::fromUtf8("Тип в сети"),                11, true,  true,  {MODBUS_DATA_TYPE}},
        {Data::DRV_DATA_TYPE,    QString::fromUtf8("Формат в приводе"),          13, true,  true,  {DRV_DATA_TYPE}},
        {SCALE_ID,               QString::fromUtf8("Цена разряда"),              14, true,  true,  {}},
        {Data::GAIN,             QString::fromUtf8("Коэффициент"),               12, true,  false, {COEFFICIENT}},
        {Data::MIN,              QString::fromUtf8("Мин."),                      9,  true,  true,  {}},
        {Data::MAX,              QString::fromUtf8("Макс."),                     9,  true,  true,  {}},
        {Data::VAR_NAME,         QString::fromUtf8("Переменная привода"),        40, false, true,  {VARIABLE_VALUE}},
        {Data::BASE,             QString::fromUtf8("Базовая величина"),          30, false, false, {}},
        {Data::NOTE,             QString::fromUtf8("Описание"),                  60, false, true,  {NOTE}},
        {Data::USER_CODE_R,      QString::fromUtf8("Код пользователя (чтение)"), 40, false, false, {}},
        {Data::USER_CODE_W,      QString::fromUtf8("Код пользователя (запись)"), 40, false, false, {}},
    };
    return columns;
}

const QVector<ExcelExchange::Column>& ExcelExchange::baseColumns() {
    using namespace Constants::JsonKeys::BaseValues;
    static const QVector<Column> columns = {
        {BASE_NAME,  QString::fromUtf8("Название базовой величины"), 40, false, true, {}},
        {UNITS,      QString::fromUtf8("Единицы"),                   10, true,  true, {}},
        {IQ_FORMAT,  QString::fromUtf8("Формат IQ"),                 10, true,  true, {}},
        {BASE_VALUE, QString::fromUtf8("Переменная / значение"),     40, false, true, {}},
        {NOTE,       QString::fromUtf8("Примечание"),                40, false, true, {}},
    };
    return columns;
}

bool ExcelExchange::exportToFile(const QString& path,
                                 const QJsonArray& data, const QJsonArray& baseValues,
                                 const QStringList& dataColumnIds, const QStringList& baseColumnIds) {
    lastError.clear();

    const QVector<Column> dataCols = pickColumns(dataColumns(), dataColumnIds);
    const QVector<Column> baseCols = pickColumns(baseColumns(), baseColumnIds);
    if (dataCols.isEmpty() && baseCols.isEmpty()) {
        lastError = QString::fromUtf8("Не выбрано ни одного столбца для экспорта");
        return false;
    }

    // Единицы базовых величин для столбца "Цена разряда"
    QHash<QString, QString> unitsByBase;
    for (const QJsonValue& val : baseValues) {
        const QJsonObject obj = val.toObject();
        unitsByBase[obj[BaseValues::BASE_NAME].toString()] = obj[BaseValues::UNITS].toString();
    }

    struct Sheet { QString name; QByteArray xml; };
    QVector<Sheet> sheets;

    if (!dataCols.isEmpty()) {
        QVector<OutRow> rows;
        for (const QJsonValue& val : data) {
            const QJsonObject obj = val.toObject();
            const QString paramType = obj[Data::PARAM_TYPE].toString();
            OutRow row;

            if (paramType == Constants::ParamType::TITLE) {
                row.merged = true;
                OutCell cell;
                cell.text = obj[Data::GROUP_NAME].toString().trimmed();
                row.cells << cell;
                rows << row;
                continue;
            }

            for (const Column& column : dataCols) {
                OutCell cell;
                cell.style = column.center ? STYLE_CENTER : STYLE_LEFT;

                if (column.id == SCALE_ID) {
                    // Цена разряда: коэффициент 10 -> "0.1 Гц"
                    const QString units = unitsByBase.value(obj[Data::BASE].toString()).trimmed();
                    bool ok;
                    const double gain = parseNumber(obj[Data::GAIN].toString(), &ok);
                    QString scale = (ok && gain != 0.0) ? formatNumber(1.0 / gain) : QString();
                    if (!units.isEmpty()) {
                        scale = scale.isEmpty() ? units : scale + " " + units;
                    }
                    cell.text = scale;
                } else if (column.id == Data::ACCESS_TYPE) {
                    cell.text = accessToExcel(obj[Data::ACCESS_TYPE].toString());
                } else if (column.id == Data::ADDRESS_DEC) {
                    cell.text = obj[Data::ADDRESS_DEC].toString().trimmed();
                    bool ok;
                    cell.text.toInt(&ok);
                    cell.isNumber = ok;
                } else if (column.id == Data::VAR_NAME && paramType == Constants::ParamType::USER) {
                    cell.text = ""; // У пользовательского параметра вместо переменной - код
                } else {
                    cell.text = obj[column.id].toString();
                }
                row.cells << cell;
            }
            rows << row;
        }
        sheets << Sheet{dataSheetName(), sheetXml(dataCols, rows)};
    }

    if (!baseCols.isEmpty()) {
        QVector<OutRow> rows;
        for (const QJsonValue& val : baseValues) {
            const QJsonObject obj = val.toObject();
            OutRow row;
            for (const Column& column : baseCols) {
                OutCell cell;
                cell.style = column.center ? STYLE_CENTER : STYLE_LEFT;
                cell.text = obj[column.id].toString();
                row.cells << cell;
            }
            rows << row;
        }
        sheets << Sheet{baseSheetName(), sheetXml(baseCols, rows)};
    }

    // Служебные файлы книги
    QByteArray contentTypes = QByteArray(XML_HEADER) +
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>";

    QByteArray workbookRels = QByteArray(XML_HEADER) +
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">";

    QByteArray workbookSheets;
    QByteArray definedNames;

    for (int i = 0; i < sheets.size(); i++) {
        const QString n = QString::number(i + 1);
        contentTypes += QString("<Override PartName=\"/xl/worksheets/sheet%1.xml\" "
                                "ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>")
                            .arg(n).toUtf8();
        workbookRels += QString("<Relationship Id=\"rId%1\" "
                                "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" "
                                "Target=\"worksheets/sheet%1.xml\"/>").arg(n).toUtf8();
        workbookSheets += QString("<sheet name=\"%1\" sheetId=\"%2\" r:id=\"rId%2\"/>")
                              .arg(sheets[i].name.toHtmlEscaped(), n).toUtf8();

        // Excel хранит диапазон автофильтра еще и в скрытом имени
        const int colCount = (sheets[i].name == dataSheetName()) ? dataCols.size() : baseCols.size();
        const int rowCount = (sheets[i].name == dataSheetName()) ? data.size() + 1 : baseValues.size() + 1;
        definedNames += QString("<definedName name=\"_xlnm._FilterDatabase\" localSheetId=\"%1\" hidden=\"1\">'%2'!$A$1:$%3$%4</definedName>")
                            .arg(i).arg(sheets[i].name.toHtmlEscaped(), columnLetters(colCount)).arg(rowCount).toUtf8();
    }
    contentTypes += "</Types>";

    const QString stylesId = QString::number(sheets.size() + 1);
    workbookRels += QString("<Relationship Id=\"rId%1\" "
                            "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" "
                            "Target=\"styles.xml\"/>").arg(stylesId).toUtf8();
    workbookRels += "</Relationships>";

    QByteArray workbook = QByteArray(XML_HEADER) +
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\" "
        "xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets>" + workbookSheets + "</sheets>"
        "<definedNames>" + definedNames + "</definedNames>"
        "</workbook>";

    QByteArray rootRels = QByteArray(XML_HEADER) +
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" "
        "Target=\"xl/workbook.xml\"/>"
        "</Relationships>";

    // Пишем во временный буфер, чтобы при ошибке не испортить существующий файл
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    {
        QZipWriter zip(&buffer);
        zip.setCompressionPolicy(QZipWriter::AlwaysCompress);
        zip.addFile("[Content_Types].xml", contentTypes);
        zip.addFile("_rels/.rels", rootRels);
        zip.addFile("xl/workbook.xml", workbook);
        zip.addFile("xl/_rels/workbook.xml.rels", workbookRels);
        zip.addFile("xl/styles.xml", stylesXml());
        for (int i = 0; i < sheets.size(); i++) {
            zip.addFile(QString("xl/worksheets/sheet%1.xml").arg(i + 1), sheets[i].xml);
        }
        zip.close();
        if (zip.status() != QZipWriter::NoError) {
            lastError = QString::fromUtf8("Не удалось сформировать файл Excel");
            return false;
        }
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        lastError = QString::fromUtf8("Не удалось открыть файл: %1\nОшибка: %2").arg(path, file.errorString());
        return false;
    }
    if (file.write(buffer.data()) != buffer.data().size()) {
        lastError = QString::fromUtf8("Не удалось записать файл: %1\nОшибка: %2").arg(path, file.errorString());
        return false;
    }
    return true;
}

bool ExcelExchange::importFromFile(const QString& path,
                                   const QStringList& dataColumnIds, const QStringList& baseColumnIds,
                                   const QJsonArray& currentBaseValues,
                                   QJsonArray& data, bool& hasData,
                                   QJsonArray& baseValues, bool& hasBase) {
    lastError.clear();
    hasData = false;
    hasBase = false;
    data = QJsonArray();
    baseValues = QJsonArray();

    QZipReader zip(path);
    if (!zip.isReadable() || zip.status() != QZipReader::NoError) {
        lastError = QString::fromUtf8("Не удалось открыть файл Excel: %1").arg(path);
        return false;
    }

    const QByteArray workbookXml = zip.fileData("xl/workbook.xml");
    if (workbookXml.isEmpty()) {
        lastError = QString::fromUtf8("Файл не похож на книгу Excel (.xlsx): %1").arg(path);
        return false;
    }

    // Связи книги: rId -> путь к листу
    QHash<QString, QString> relTargets;
    {
        QXmlStreamReader xml(zip.fileData("xl/_rels/workbook.xml.rels"));
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == u"Relationship") {
                QString target = xml.attributes().value("Target").toString();
                target = target.startsWith('/') ? target.mid(1) : "xl/" + target;
                relTargets[xml.attributes().value("Id").toString()] = target;
            }
        }
    }

    // Листы книги по порядку: имя и путь
    QVector<QPair<QString, QString>> sheetList;
    {
        QXmlStreamReader xml(workbookXml);
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == u"sheet") {
                QString relId;
                for (const QXmlStreamAttribute& attr : xml.attributes()) {
                    if (attr.name() == u"id") relId = attr.value().toString(); // r:id
                }
                sheetList << qMakePair(xml.attributes().value("name").toString(), relTargets.value(relId));
            }
        }
    }

    const QStringList sharedStrings = readSharedStrings(zip.fileData("xl/sharedStrings.xml"));

    // Ищем листы по имени; если имени нет - лист с данными первый из остальных
    QString dataSheetPath;
    QString baseSheetPath;
    for (const auto& sheet : std::as_const(sheetList)) {
        if (sheet.first.compare(dataSheetName(), Qt::CaseInsensitive) == 0) dataSheetPath = sheet.second;
        if (sheet.first.compare(baseSheetName(), Qt::CaseInsensitive) == 0) baseSheetPath = sheet.second;
    }
    if (dataSheetPath.isEmpty()) {
        for (const auto& sheet : std::as_const(sheetList)) {
            if (sheet.second != baseSheetPath) {
                dataSheetPath = sheet.second;
                break;
            }
        }
    }

    // --- Лист "Базовые величины"
    if (!baseColumnIds.isEmpty() && !baseSheetPath.isEmpty()) {
        const InSheet sheet = readSheet(zip.fileData(baseSheetPath), sharedStrings);
        const QHash<QString, int> colOf = mapHeader(sheet, baseColumns());

        for (auto rowIt = sheet.cells.begin(); rowIt != sheet.cells.end(); ++rowIt) {
            if (rowIt == sheet.cells.begin()) continue; // Шапка

            QJsonObject obj;
            bool hasAny = false;
            for (const Column& column : baseColumns()) {
                QString value;
                if (baseColumnIds.contains(column.id) && colOf.contains(column.id)) {
                    value = rowIt.value().value(colOf[column.id]).trimmed();
                }
                hasAny = hasAny || !value.isEmpty();
                obj[column.id] = value;
            }
            if (hasAny) baseValues.append(obj);
        }
        hasBase = true;
    }

    // --- Лист "Карта регистров Modbus"
    if (!dataColumnIds.isEmpty()) {
        if (dataSheetPath.isEmpty()) {
            lastError = QString::fromUtf8("В файле не найден лист \"%1\"").arg(dataSheetName());
            return false;
        }

        const InSheet sheet = readSheet(zip.fileData(dataSheetPath), sharedStrings);
        const QHash<QString, int> colOf = mapHeader(sheet, dataColumns());

        bool anyKnownColumn = false;
        for (const QString& id : dataColumnIds) {
            anyKnownColumn = anyKnownColumn || colOf.contains(id);
        }
        if (!anyKnownColumn) {
            lastError = QString::fromUtf8("На листе \"%1\" не найдено ни одного из выбранных заголовков").arg(dataSheetName());
            return false;
        }

        // Единицы -> базовые величины, чтобы угадать базу по "Цене разряда",
        // если столбец "Базовая величина" не импортируется
        QHash<QString, QStringList> basesByUnits;
        for (const QJsonValue& val : hasBase ? std::as_const(baseValues) : currentBaseValues) {
            const QJsonObject obj = val.toObject();
            basesByUnits[obj[BaseValues::UNITS].toString().trimmed()] << obj[BaseValues::BASE_NAME].toString();
        }

        auto cellOf = [&colOf, &dataColumnIds](const QMap<int, QString>& row, const QString& id) -> QString {
            if (!dataColumnIds.contains(id) || !colOf.contains(id)) return QString();
            return row.value(colOf[id]).trimmed();
        };

        static const QRegularExpression scaleRe(R"(^\s*([-+]?\d+(?:[.,]\d+)?(?:[eE][-+]?\d+)?)?\s*(.*)$)");

        for (auto rowIt = sheet.cells.begin(); rowIt != sheet.cells.end(); ++rowIt) {
            if (rowIt == sheet.cells.begin()) continue; // Шапка
            const QMap<int, QString>& row = rowIt.value();

            QJsonObject obj;
            for (const QString& key : Constants::JsonKeys::Data::MAIN_KEYS()) obj[key] = "";
            for (const QString& key : Constants::JsonKeys::Data::HIDDEN_KEYS()) obj[key] = "";

            // Заголовок группы: объединенная строка или единственная непустая ячейка в столбце A
            const bool isTitle = sheet.mergedFromFirstCol.value(rowIt.key()) > 1 ||
                                 (row.size() == 1 && row.firstKey() == 1);
            if (isTitle) {
                obj[Data::GROUP_NAME] = row.first().trimmed();
                obj[Data::PARAM_TYPE] = Constants::ParamType::TITLE;
                data.append(obj);
                continue;
            }

            obj[Data::GROUP_NAME] = cellOf(row, Data::GROUP_NAME);
            obj[Data::ACCESS_TYPE] = accessFromExcel(cellOf(row, Data::ACCESS_TYPE));
            obj[Data::MODBUS_DATA_TYPE] = cellOf(row, Data::MODBUS_DATA_TYPE);
            obj[Data::DRV_DATA_TYPE] = cellOf(row, Data::DRV_DATA_TYPE);
            obj[Data::MIN] = cellOf(row, Data::MIN);
            obj[Data::MAX] = cellOf(row, Data::MAX);
            obj[Data::VAR_NAME] = cellOf(row, Data::VAR_NAME);
            obj[Data::BASE] = cellOf(row, Data::BASE);
            obj[Data::NOTE] = cellOf(row, Data::NOTE);
            obj[Data::USER_CODE_R] = cellOf(row, Data::USER_CODE_R);
            obj[Data::USER_CODE_W] = cellOf(row, Data::USER_CODE_W);

            // Адрес: достаточно одного из столбцов, второй пересчитываем
            int address = -1;
            const QString dec = cellOf(row, Data::ADDRESS_DEC);
            const QString hex = cellOf(row, Data::ADDRESS_HEX);
            bool ok = false;
            if (!dec.isEmpty()) {
                address = dec.toInt(&ok);
                if (!ok) address = -1;
            }
            if (address < 0 && !hex.isEmpty()) {
                address = AddressConverter::hexToDec(hex);
            }
            if (address >= 0) {
                obj[Data::ADDRESS_DEC] = QString::number(address);
                obj[Data::ADDRESS_HEX] = AddressConverter::decToHex(address);
            }

            // Коэффициент: из своего столбца или из "Цены разряда" (0.1 Гц -> 10.0)
            QString gain = cellOf(row, Data::GAIN);
            const QString scale = cellOf(row, SCALE_ID);
            QString units;
            if (!scale.isEmpty()) {
                const QRegularExpressionMatch match = scaleRe.match(scale);
                units = match.captured(2).trimmed();
                if (gain.isEmpty() && !match.captured(1).isEmpty()) {
                    const double price = parseNumber(match.captured(1), &ok);
                    if (ok && price != 0.0) gain = formatGain(1.0 / price);
                }
            }
            obj[Data::GAIN] = gain;

            // Базовая величина не задана - берем единственную базу с такими единицами
            if (obj[Data::BASE].toString().isEmpty() && !units.isEmpty() &&
                basesByUnits.value(units).size() == 1) {
                obj[Data::BASE] = basesByUnits.value(units).first();
            }

            // Без имени переменной параметр может быть только пользовательским,
            // иначе генератор получил бы "reg->data = ;"
            const bool hasUserCode = !obj[Data::USER_CODE_R].toString().isEmpty() ||
                                     !obj[Data::USER_CODE_W].toString().isEmpty();
            // Метка "userCode (...)" в столбце переменной - тоже пользовательский параметр
            const bool isUser = hasUserCode || obj[Data::VAR_NAME].toString().isEmpty() ||
                                obj[Data::VAR_NAME].toString().startsWith("userCode", Qt::CaseInsensitive);
            obj[Data::PARAM_TYPE] = isUser ? Constants::ParamType::USER : Constants::ParamType::COMMON;
            if (isUser) obj[Data::VAR_NAME] = "";

            data.append(obj);
        }
        hasData = true;
    }

    if (!hasData && !hasBase) {
        lastError = QString::fromUtf8("В файле не найдено выбранных листов");
        return false;
    }
    return true;
}

QStringList ExcelExchange::groupNames(const QJsonArray& data) {
    QStringList groups;
    bool hasNoGroupRows = false;
    bool titleSeen = false;
    for (const QJsonValue& val : data) {
        const QJsonObject obj = val.toObject();
        if (obj[Data::PARAM_TYPE].toString() == Constants::ParamType::TITLE) {
            titleSeen = true;
            const QString name = obj[Data::GROUP_NAME].toString().trimmed();
            if (!groups.contains(name)) groups << name;
        } else if (!titleSeen) {
            hasNoGroupRows = true;
        }
    }
    if (hasNoGroupRows) groups.prepend(noGroupName());
    return groups;
}

QJsonArray ExcelExchange::filterGroups(const QJsonArray& data, const QStringList& selectedGroups) {
    QJsonArray result;
    QString currentGroup = noGroupName();
    for (const QJsonValue& val : data) {
        const QJsonObject obj = val.toObject();
        if (obj[Data::PARAM_TYPE].toString() == Constants::ParamType::TITLE) {
            currentGroup = obj[Data::GROUP_NAME].toString().trimmed();
        }
        if (selectedGroups.contains(currentGroup)) {
            result.append(obj);
        }
    }
    return result;
}
