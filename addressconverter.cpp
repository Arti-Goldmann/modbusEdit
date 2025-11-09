#include "addressconverter.h"

namespace AddressConverter {

QString decToHex(int decValue) {
    QString hexDigits = QString("%1").arg(decValue, 0, 16, QChar('0')).toUpper();
    return QString("0x%1").arg(hexDigits);
}

int hexToDec(const QString& hexText) {
    QString cleanHex = hexText.trimmed();

    // Убираем префикс 0x если есть
    if (cleanHex.startsWith("0x", Qt::CaseInsensitive)) {
        cleanHex = cleanHex.mid(2);
    }

    bool ok;
    int decValue = cleanHex.toInt(&ok, 16);

    return ok ? decValue : -1;
}

bool isValidHex(const QString& hexText) {
    return hexToDec(hexText) != -1;
}

} // namespace AddressConverter
