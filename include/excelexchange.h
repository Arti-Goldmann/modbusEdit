#ifndef EXCELEXCHANGE_H
#define EXCELEXCHANGE_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonArray>

// Экспорт/импорт профиля в файл Excel (.xlsx).
// Формат листа "Карта регистров Modbus" повторяет таблицу MODBUS-MAP.xlsx:
// серая шапка, заголовки групп объединены на всю ширину, шрифт Times New Roman.
// Второй лист "Базовые величины" содержит таблицу базовых величин.
// Сторонние библиотеки не нужны: xlsx - это zip с xml, используем QZipReader/QZipWriter из QtCore.
class ExcelExchange
{
public:
    // Описание столбца Excel
    struct Column {
        QString id;          // Ключ JSON (или служебный ключ, например "scale")
        QString header;      // Текст заголовка в Excel
        double width;        // Ширина столбца в Excel
        bool center;         // Выравнивание по центру (иначе по левому краю)
        bool defaultOn;      // Отмечен ли столбец по умолчанию
        QStringList aliases; // Другие варианты заголовка, которые понимаем при импорте
    };

    // Служебный столбец "Цена разряда" = 1 / коэффициент + единицы базовой величины
    static constexpr const char* SCALE_ID = "scale";

    static const QVector<Column>& dataColumns();
    static const QVector<Column>& baseColumns();

    static QString dataSheetName() { return QString::fromUtf8("Карта регистров Modbus"); }
    static QString baseSheetName() { return QString::fromUtf8("Базовые величины"); }

    // Экспорт. data/baseValues - массивы в формате JSON-профиля.
    bool exportToFile(const QString& path,
                      const QJsonArray& data, const QJsonArray& baseValues,
                      const QStringList& dataColumnIds, const QStringList& baseColumnIds);

    // Импорт. Если лист не выбран или не найден в файле, соответствующий has* = false
    // и таблицу трогать не нужно. currentBaseValues - текущие базовые величины,
    // по их единицам подбирается база, если лист базовых величин не импортируется.
    bool importFromFile(const QString& path,
                        const QStringList& dataColumnIds, const QStringList& baseColumnIds,
                        const QJsonArray& currentBaseValues,
                        QJsonArray& data, bool& hasData,
                        QJsonArray& baseValues, bool& hasBase);

    QString getLastError() const { return lastError; }

    // Группы параметров (строки-заголовки) по порядку. Параметры до первого
    // заголовка относятся к псевдогруппе noGroupName().
    static QString noGroupName() { return QString::fromUtf8("(без группы)"); }
    static QStringList groupNames(const QJsonArray& data);

    // Оставить только строки выбранных групп (вместе с их заголовками)
    static QJsonArray filterGroups(const QJsonArray& data, const QStringList& selectedGroups);

private:
    QString lastError;
};

#endif // EXCELEXCHANGE_H
