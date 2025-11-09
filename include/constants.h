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

        // Типы данных ModBus
    namespace drvDataType {
        constexpr const char* FRACT = "FRACT";
        constexpr const char* INT = "INT";

        inline QStringList toStringList() {
            return {FRACT, INT};
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
