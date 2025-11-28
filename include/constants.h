#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>
#include <QStringList>

// Пространство имен для констант приложения
namespace Constants {

    // Типы доступа к параметрам
    namespace AccessType {
        constexpr const char* READ_ONLY = "R";
        constexpr const char* READ_WRITE = "RW";
        constexpr const char* READ_WRITE_IN_STOP = "RW (in STOP)";

        // Вспомогательная функция для получения списка типов доступа
        inline QStringList toStringList() {
            return {READ_WRITE, READ_WRITE_IN_STOP, READ_ONLY};
        }
    }

    // Типы параметров
    namespace ParamType {
        constexpr const char* COMMON = "commonType";
        constexpr const char* USER = "userType";
        constexpr const char* TITLE = "titleType";
    }

    // Типы данных ModBus
    namespace ModBusDataType {
        constexpr const char* INT16 = "int16";
        constexpr const char* UINT16 = "Uint16";

        inline QStringList toStringList() {
            return {INT16, UINT16};
        }
    }

    // Типы данных привода
    namespace drvDataType {
        // IQ типы (числа с фиксированной точкой)
        constexpr const char* IQ31 = "IQ31";
        constexpr const char* IQ30 = "IQ30";
        constexpr const char* IQ29 = "IQ29";
        constexpr const char* IQ28 = "IQ28";
        constexpr const char* IQ27 = "IQ27";
        constexpr const char* IQ26 = "IQ26";
        constexpr const char* IQ25 = "IQ25";
        constexpr const char* IQ24 = "IQ24";
        constexpr const char* IQ23 = "IQ23";
        constexpr const char* IQ22 = "IQ22";
        constexpr const char* IQ21 = "IQ21";
        constexpr const char* IQ20 = "IQ20";
        constexpr const char* IQ19 = "IQ19";
        constexpr const char* IQ18 = "IQ18";
        constexpr const char* IQ17 = "IQ17";
        constexpr const char* IQ16 = "IQ16";
        constexpr const char* IQ15 = "IQ15";
        constexpr const char* IQ14 = "IQ14";
        constexpr const char* IQ13 = "IQ13";
        constexpr const char* IQ12 = "IQ12";
        constexpr const char* IQ11 = "IQ11";
        constexpr const char* IQ10 = "IQ10";
        constexpr const char* IQ9 = "IQ9";
        constexpr const char* IQ8 = "IQ8";
        constexpr const char* IQ7 = "IQ7";
        constexpr const char* IQ6 = "IQ6";
        constexpr const char* IQ5 = "IQ5";
        constexpr const char* IQ4 = "IQ4";
        constexpr const char* IQ3 = "IQ3";
        constexpr const char* IQ2 = "IQ2";
        constexpr const char* IQ1 = "IQ1";
        constexpr const char* IQ0 = "IQ0";

        // Float тип
        constexpr const char* FLOAT = "float";

        // StandartNum типы (целые числа)
        constexpr const char* INT8 = "int8";
        constexpr const char* UINT8 = "Uint8";
        constexpr const char* INT16 = "int16";
        constexpr const char* UINT16 = "Uint16";
        constexpr const char* INT32 = "int32";
        constexpr const char* UINT32 = "Uint32";

        inline QStringList toStringList() {
            return {
                // IQ типы
                IQ31, IQ30, IQ29, IQ28, IQ27, IQ26, IQ25, IQ24,
                IQ23, IQ22, IQ21, IQ20, IQ19, IQ18, IQ17, IQ16,
                IQ15, IQ14, IQ13, IQ12, IQ11, IQ10, IQ9, IQ8,
                IQ7, IQ6, IQ5, IQ4, IQ3, IQ2, IQ1, IQ0,
                // Float
                FLOAT,
                // StandartNum типы
                INT8, UINT8, INT16, UINT16, INT32, UINT32
            };
        }

        // Вспомогательная функция для проверки, является ли тип StandartNum (целочисленным)
        inline bool isStandartNum(const QString& type) {
            return type == INT8 || type == UINT8 ||
                   type == INT16 || type == UINT16 ||
                   type == INT32 || type == UINT32;
        }

        // Вспомогательная функция для проверки, является ли тип IQ
        inline bool isIQ(const QString& type) {
            return type.startsWith("IQ");
        }

        // Вспомогательная функция для проверки, является ли тип float
        inline bool isFloat(const QString& type) {
            return type == FLOAT;
        }

        // Вспомогательная функция для проверки, является ли тип signed
        inline bool isSigned(const QString& type) {
            return type == INT8 || type == INT16 || type == INT32;
        }

        // Вспомогательная функция для проверки, является ли тип unsigned
        inline bool isUnsigned(const QString& type) {
            return type == UINT8 || type == UINT16 || type == UINT32;
        }
    }

    //Смещение внутри корневого элемента таблицы для размещения пользовательского кода
    enum UserCodeOffsetInTable {
        R = 0, //Чтение
        W = 1, //Запись
    };

    // Заголовки колонок таблиц
    namespace TableHeaders {
        // Заголовки основной таблицы с данными
        inline const QStringList DATA_TABLE() {
            return {
                "Название группы параметров / параметра",
                "Тип доступа",
                "Тип данных ModBus",
                "Тип данных привода",
                "Коэффициент",
                "Адрес (дес.)",
                "Адрес (hex.)",
                "Переменная / значение",
                "Мин.",
                "Макс.",
                "Базовая величина",
                "Примечание"
            };
        }

        // Заголовки таблицы базовых величин
        inline const QStringList BASE_VALUES_ARR() {
            return {
                "Название базовой величины",
                "Единицы",
                "Формат IQ",
                "Переменная / значение",
                "Примечание"
            };
        }

        // Названия отдельных колонок для быстрого доступа
        constexpr const char* GROUP_NAME = "Название группы параметров / параметра";
        constexpr const char* ACCESS_TYPE = "Тип доступа";
        constexpr const char* MODBUS_DATA_TYPE = "Тип данных ModBus";
        constexpr const char* DRV_DATA_TYPE = "Тип данных СУ";
        constexpr const char* COEFFICIENT = "Коэффициент";
        constexpr const char* ADDRESS_DEC = "Адрес (дес.)";
        constexpr const char* ADDRESS_HEX = "Адрес (hex.)";
        constexpr const char* VARIABLE_VALUE = "Переменная / значение";
        constexpr const char* MIN = "Мин.";
        constexpr const char* MAX = "Макс.";
        constexpr const char* BASE_VALUE = "Базовая величина";
        constexpr const char* NOTE = "Примечание";
    }

    // Ключи JSON
    namespace JsonKeys {
        // Ключи массива DATA
        namespace Data {
            // Название массива в корневом JSON объекте
            constexpr const char* ARRAY_NAME = "data";

            // Основные ключи (соответствуют колонкам таблицы)
            inline const QStringList MAIN_KEYS() {
                return {"groupName", "accessType", "modBusDataType", "drvDataType", "gain",
                        "addressDec", "addressHex", "varName", "min", "max", "base", "note"};
            }

            // Скрытые ключи (не отображаются в таблице)
            inline const QStringList HIDDEN_KEYS() {
                return {"paramType", "userCode_R", "userCode_W"};
            }

            // Отдельные ключи для прямого доступа
            constexpr const char* GROUP_NAME = "groupName";
            constexpr const char* ACCESS_TYPE = "accessType";
            constexpr const char* MODBUS_DATA_TYPE = "modBusDataType";
            constexpr const char* DRV_DATA_TYPE = "drvDataType";
            constexpr const char* GAIN = "gain";
            constexpr const char* ADDRESS_DEC = "addressDec";
            constexpr const char* ADDRESS_HEX = "addressHex";
            constexpr const char* VAR_NAME = "varName";
            constexpr const char* MIN = "min";
            constexpr const char* MAX = "max";
            constexpr const char* BASE = "base";
            constexpr const char* NOTE = "note";
            constexpr const char* PARAM_TYPE = "paramType";
            constexpr const char* USER_CODE_R = "userCode_R";
            constexpr const char* USER_CODE_W = "userCode_W";
        }

        // Ключи массива BASE_VALUES
        namespace BaseValues {
            // Название массива в корневом JSON объекте
            constexpr const char* ARRAY_NAME = "baseValues";

            // Ключи (соответствуют колонкам таблицы)
            inline const QStringList KEYS() {
                return {"baseName", "units", "IQformat", "baseValue", "note"};
            }

            // Отдельные ключи для прямого доступа
            constexpr const char* BASE_NAME = "baseName";
            constexpr const char* UNITS = "units";
            constexpr const char* IQ_FORMAT = "IQformat";
            constexpr const char* BASE_VALUE = "baseValue";
            constexpr const char* NOTE = "note";
        }
    }

    // UI текст
    namespace UiText {
        constexpr const char* PASTE_CODE = "*** PASTE YOUR CODE ***";
        constexpr const char* USER_CODE_OK = "*** USER CODE ✓ ***";
        constexpr const char* USER_CODE = "USER CODE";
        constexpr const char* READ_LABEL = "на чтение (R)";
        constexpr const char* WRITE_LABEL = "на запись (W)";
    }

} // namespace Constants

#endif // CONSTANTS_H
