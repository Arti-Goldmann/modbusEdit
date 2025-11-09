#ifndef ADDRESSCONVERTER_H
#define ADDRESSCONVERTER_H

#include <QString>

// Утилиты для конвертации адресов между десятичным и шестнадцатеричным форматами
namespace AddressConverter {
    // Конвертирует десятичное значение в hex строку с префиксом "0x"
    QString decToHex(int decValue);

    // Конвертирует hex строку (с префиксом "0x" или без) в десятичное значение
    // Возвращает -1 при ошибке парсинга
    int hexToDec(const QString& hexText);

    // Проверяет валидность hex строки
    bool isValidHex(const QString& hexText);
}

#endif // ADDRESSCONVERTER_H
