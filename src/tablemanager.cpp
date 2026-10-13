#include "tablemanager.h"
#include "usercodeeditordialog.h"
#include "addressconverter.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QColor>
#include <QPalette>
#include <QTimer>
#include <QPointer>
#include <algorithm>
#include <QMessageBox>
#include <QShortcut>
#include <QKeySequence>
#include <climits>

TableManager::TableManager(QObject *parent, QTableWidget* dataTable, QTableWidget* baseValuesTable)
    : QObject(parent),
    dataTable(dataTable),
    baseValuesTable(baseValuesTable)
{
}

TableManager::~TableManager() {
    clearCutBuffer();
}

void TableManager::clearCutBuffer() {
    for (auto& rowItems : cutBuffer) {
        qDeleteAll(rowItems);
    }
    cutBuffer.clear();
    cutTable = nullptr;
}

void TableManager::cutRow(QTableWidget* table, int row) {
    if (!isDataRow(table, row)) return;

    int endRow = row + 1;

    // Для заголовка с параметрами спрашиваем, что именно вырезать
    if (table == dataTable && rowTypeOf(table, row) == Constants::ParamType::TITLE
        && groupEndRow(table, row) > row + 1) {
        QMessageBox msgBox(qobject_cast<QWidget*>(parent()));
        msgBox.setWindowTitle("Вырезать заголовок");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText("Вырезать вместе с параметрами группы?");

        QPushButton* withParamsButton = msgBox.addButton("Да, с параметрами", QMessageBox::YesRole);
        QPushButton* titleOnlyButton = msgBox.addButton("Нет, только заголовок", QMessageBox::NoRole);
        msgBox.addButton("Отмена", QMessageBox::RejectRole);
        msgBox.setDefaultButton(withParamsButton);
        msgBox.exec();

        if (msgBox.clickedButton() == withParamsButton) {
            endRow = groupEndRow(table, row);
        } else if (msgBox.clickedButton() != titleOnlyButton) {
            return; // Отмена
        }
    }

    // Новое вырезание заменяет старый буфер
    clearCutBuffer();
    cutTable = table;

    // clone() сохраняет текст, флаги, цвета и скрытые данные (тип строки, пользовательский код)
    for (int r = row; r < endRow; r++) {
        QVector<QTableWidgetItem*> rowItems;
        for (int col = 0; col < table->columnCount(); col++) {
            QTableWidgetItem* item = table->item(r, col);
            rowItems << (item ? item->clone() : nullptr);
        }
        cutBuffer << rowItems;
    }

    for (int r = endRow - 1; r >= row; r--) {
        table->removeRow(r);
    }

    emit dataModified();
}

void TableManager::pasteRows(QTableWidget* table, int row) {
    if (!canPaste(table)) return;

    // Вставляем над выбранной строкой; если строка не выбрана - в конец (перед плюсиком)
    int insertRow = (row >= 0 && row < table->rowCount()) ? row : table->rowCount() - 1;

    table->blockSignals(true);
    for (int i = 0; i < cutBuffer.size(); i++) {
        const int dstRow = insertRow + i;
        table->insertRow(dstRow);
        for (int col = 0; col < cutBuffer[i].size(); col++) {
            if (cutBuffer[i][col]) {
                table->setItem(dstRow, col, cutBuffer[i][col]); // Таблица забирает item себе
            }
        }
        // Заголовок занимает всю строку
        QTableWidgetItem* rootItem = table->item(dstRow, 0);
        if (rootItem && rootItem->data(Qt::UserRole).toString() == Constants::ParamType::TITLE) {
            table->setSpan(dstRow, 0, 1, table->columnCount());
        }
    }
    table->blockSignals(false);

    // Вырезанное вставляется один раз: item'ы теперь принадлежат таблице
    cutBuffer.clear();
    cutTable = nullptr;

    table->setCurrentCell(insertRow, 0);
    emit dataModified();
}

void TableManager::setupTable(QTableWidget* table) {
    //Настройка высоты строк
    table->verticalHeader()->setDefaultSectionSize(25);
    table->verticalHeader()->setFixedWidth(45);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed); //Нельзя менять высоту строки
    table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter); //Выравнивание по центру
    table->setSelectionBehavior(QAbstractItemView::SelectItems); //Поведение при выделении ячейки
    table->setSelectionMode(QAbstractItemView::SingleSelection); //Только одна ячейка
    table->setAlternatingRowColors(true); //Делает строку черно-белой через одну
    table->horizontalHeader()->setStretchLastSection(true); //Растягивает последний столбец на всю оставшуюся ширину таблицы

    //Настройка стиля
    table->setStyleSheet(
        "QHeaderView::section {"
        "    border: 1px solid black;"
        "}"
        "QTableWidget::item:selected {"
        "    background-color: transparent;"
        "    selection-background-color: transparent;"
        "    border: 2px solid black;"
        "}"
        );
}

void TableManager::addPlusRow(QTableWidget* table) {
    int row = table->rowCount();
    table->insertRow(row);

    // Объединить все столбцы в одну ячейку
    table->setSpan(row, 0, 1, table->columnCount()); // row, column, rowSpan, columnSpan

    // Создать элемент с плюсиком
    QTableWidgetItem *item = new QTableWidgetItem(" +");

    QFont font;
    font.setPointSize(18);        // размер шрифта
    font.setBold(true);           // жирный

    item->setFont(font);
    item->setForeground(QColor(0, 140, 0)); // Темно-зеленый цвет
    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Сделать нередактируемой
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);

    table->setItem(row, 0, item);
}

void TableManager::fillTable(QTableWidget* table,
                             const QJsonArray& data,
                             const QStringList& mainKeys,
                             bool configRow) {
    // Заполняем таблицу с данными
    table->setRowCount(0);

    // ОТКЛЮЧАЕМ обновление таблицы, чтобы не загружать UI
    table->setUpdatesEnabled(false);
    table->setRowCount(0);

    const bool isDataTable = (table == dataTable);
    const int colAddressHex = isDataTable
                                  ? mainKeys.indexOf(Constants::JsonKeys::Data::ADDRESS_HEX)
                                  : -1;

    int rowCounter = 0;
    for(const QJsonValue &val : std::as_const(data)) {

        QJsonObject obj = val.toObject();

        // Добавляем строку в таблицу
        table->insertRow(rowCounter);

        //Получаем строку из объекта json и кладем в соответсвующую колонку
        for(int col = 0; col < mainKeys.size(); col++) {
            QString key = mainKeys[col];
            QString str = obj[key].toString();

            // Старые профили могли хранить hex-адрес без лидирующих нулей.
            // Нормализуем только отображение в основной таблице: сам JSON-файл
            // не перезаписывается, пока пользователь явно не нажмет "Сохранить".
            if (col == colAddressHex && !str.trimmed().isEmpty()) {
                int decValue = AddressConverter::hexToDec(str);
                if (decValue != -1) {
                    str = AddressConverter::decToHex(decValue);
                }
            }

            table->setItem(rowCounter, col, new QTableWidgetItem(str));
        }

        if(configRow) { //Конфигурируем строки
            QString paramType = obj[Constants::JsonKeys::Data::PARAM_TYPE].toString();

            if(paramType == Constants::ParamType::COMMON) { //Встяавляем в соответсвующую колонку название перемеенной
                QString varName = obj[Constants::JsonKeys::Data::VAR_NAME].toString();
                QVector<QString> data = {varName};
                setRowType(paramType, rowCounter, table, data);

                // Обновляем состояние ячеек только для COMMON типа
                if(table == dataTable) {
                    updateCellsStateByDataType(rowCounter, table);
                }
            } else if(paramType == Constants::ParamType::USER) {
                //Пользовательская ячейка. Выставляем PASTE YOUR CODE в ячейке, если пустая: USER CODE
                QString userCode_R = obj[Constants::JsonKeys::Data::USER_CODE_R].toString();
                QString userCode_W = obj[Constants::JsonKeys::Data::USER_CODE_W].toString();
                QVector<QString> data = {userCode_R, userCode_W};
                QString accessType = obj[Constants::JsonKeys::Data::ACCESS_TYPE].toString();
                setRowType(paramType, rowCounter, table, data, accessType);

                // Обновляем состояние ячеек только для USER типа
                if(table == dataTable) {
                    updateCellsStateByDataType(rowCounter, table);
                }
            } else if(paramType == Constants::ParamType::TITLE) {
                //Заголовок - объединяем все столбцы
                QString groupName = obj[Constants::JsonKeys::Data::GROUP_NAME].toString();
                QVector<QString> data = {groupName};
                setRowType(paramType, rowCounter, table, data);
            }
        }
        rowCounter++;

        // Каждые 100 строк обновляем UI и прогресс
        if (rowCounter % 100 == 0) {
            QApplication::processEvents();
        }
    }

    addPlusRow(table); //Добавляем последнюю строку с плюсиком

    // ВКЛЮЧАЕМ обновление обратно
    table->setUpdatesEnabled(true);
}

void TableManager::setRowType(const QString& rowType, int rowIndex, QTableWidget* table,
                              const QVector<QString>& data, const QString& accessType) {
    if (!table) return;

    //Сохраняем инфу с типом параметра в 0 колонке
    QTableWidgetItem *itemRoot = table->item(rowIndex, 0);
    if (!itemRoot) {
        itemRoot = new QTableWidgetItem();
        table->setItem(rowIndex, 0, itemRoot);
    }
    itemRoot->setData(Qt::UserRole, rowType);

    // Снимаем объединение столбцов (на случай если раньше был TITLE)
    if (rowType != Constants::ParamType::TITLE) {
        table->setSpan(rowIndex, 0, 1, 1);

        // Сбрасываем форматирование первой ячейки к шрифту таблицы
        // (иначе после заголовка оставался увеличенный жирный шрифт)
        itemRoot->setFont(table->font());
        itemRoot->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Создаем пустые ячейки для всех столбцов (если их нет)
        for (int col = 1; col < table->columnCount(); col++) {
            if (!table->item(rowIndex, col)) {
                table->setItem(rowIndex, col, new QTableWidgetItem(""));
            }
        }
    }

    int columnIndex = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::VARIABLE_VALUE);
    if (columnIndex == -1) return;

    //В колонку "Переменная / значение" устаналиваем текст и настройки в зависимости от типа
    QTableWidgetItem *item = table->item(rowIndex, columnIndex);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(rowIndex, columnIndex, item);
    }

    if(rowType == Constants::ParamType::COMMON) {
        // Если строку создали вручную, а не загрузили из профиля,
        // сразу заполняем поля с выпадающими списками значениями по умолчанию.
        if (data.isEmpty()) {
            applyDefaultComboValues(rowIndex, table);
        }

        QString varName = !data.isEmpty() ? data[0] : "";
        item->setText(varName); //Переменная хранится в 0 элементе
        item->setForeground(QBrush()); // Сбрасываем зеленый цвет пользовательского кода
        item->setFlags(item->flags() | Qt::ItemIsEditable); // делаем редактируемым

    } else if (rowType == Constants::ParamType::USER) {
        // При переключении новой строки в пользовательский тип сохраняем
        // те же значения по умолчанию, чтобы комбобоксы не оставались пустыми.
        if (data.isEmpty()) {
            applyDefaultComboValues(rowIndex, table);
        }

        bool isEmpty = true;
        //В корневой элемент сохраняем пользовательский код
        if(Constants::AccessType::isReadOnlyCode(accessType)) { //Если R или RW (чтение), то тогда нам должны были передать только посльзовательский код на чтение
            QString userCode_R = !data.isEmpty() ? data[Constants::UserCodeOffsetInTable::R] : "";
            if(userCode_R != "") isEmpty = false;
            itemRoot->setData(Qt::UserRole + 1 + Constants::UserCodeOffsetInTable::R, userCode_R);

        } else if(accessType == Constants::AccessType::READ_WRITE || accessType == Constants::AccessType::READ_WRITE_IN_STOP) { //Если RW, то тогда должен быть пользовательский код и на чтение и на запись
            QString userCode_R = !data.isEmpty() ? data[Constants::UserCodeOffsetInTable::R] : "";
            QString userCode_W = !data.isEmpty() ? data[Constants::UserCodeOffsetInTable::W] : "";

            if(userCode_R != "" || userCode_W != "") isEmpty = false;

            itemRoot->setData(Qt::UserRole + 1 + Constants::UserCodeOffsetInTable::R, userCode_R);
            itemRoot->setData(Qt::UserRole + 1 + Constants::UserCodeOffsetInTable::W, userCode_W);
        }

        if(isEmpty) {
            item->setText(Constants::UiText::PASTE_CODE);
            item->setForeground(QBrush());  // Пустая кисть = дефолтный цвет
        } else {
            item->setText(Constants::UiText::USER_CODE_OK);
            item->setForeground(QBrush(QColor(0, 140, 0))); // Темно-зеленый цвет
        }
        item->setFlags(item->flags() & ~Qt::ItemIsEditable); // убираем возможность редактирования

    } else if (rowType == Constants::ParamType::TITLE) {
        for (int col = 1; col < table->columnCount(); col++) {
            QTableWidgetItem* extraItem = table->item(rowIndex, col);
            if (!extraItem) {
                extraItem = new QTableWidgetItem();
                table->setItem(rowIndex, col, extraItem);
            }
            // Сбрасываем текст и оформление (зеленый цвет кода, серый фон
            // неактивных ячеек), чтобы при смене типа обратно строка была чистой
            extraItem->setText("");
            extraItem->setForeground(QBrush());
            extraItem->setBackground(QBrush());
            extraItem->setFlags(extraItem->flags() | Qt::ItemIsEditable);
        }

        // Объединяем все столбцы в одну ячейку
        table->setSpan(rowIndex, 0, 1, table->columnCount());

        // Текст заголовка: из переданных данных, а при смене типа через меню
        // оставляем название, которое уже было в первой ячейке
        if (!data.isEmpty()) {
            itemRoot->setText(data[0]);
        }

        // Настраиваем внешний вид. Шрифт считаем от шрифта таблицы, чтобы
        // повторная смена типа не увеличивала его каждый раз
        QFont font = table->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        itemRoot->setFont(font);
        itemRoot->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Делаем редактируемым
        itemRoot->setFlags(itemRoot->flags() | Qt::ItemIsEditable);
    }

    // Активность ячеек "Коэффициент"/"Базовая величина" зависит от типа данных привода -
    // пересчитываем, иначе после смены типа строки оставался старый серый фон
    if (table == dataTable && rowType != Constants::ParamType::TITLE) {
        updateCellsStateByDataType(rowIndex, table);
    }
}

void TableManager::applyDefaultComboValues(int rowIndex, QTableWidget* table) {
    // Дефолты нужны только основной таблице с картой регистров.
    // Таблицу базовых величин здесь не трогаем.
    if (table != dataTable || rowIndex < 0 || rowIndex >= table->rowCount()) return;

    // Универсальная установка значения: создаем ячейку при необходимости,
    // но не перезаписываем уже введенные или загруженные из профиля данные.
    auto setTextIfEmpty = [table, rowIndex](int col, const QString& value) {
        if (col < 0 || col >= table->columnCount() || value.isEmpty()) return;

        QTableWidgetItem* item = table->item(rowIndex, col);
        if (!item) {
            item = new QTableWidgetItem();
            table->setItem(rowIndex, col, item);
        }

        if (item->text().trimmed().isEmpty()) {
            item->setText(value);
        }
    };

    // Для новых записей сразу выбираем первые варианты из выпадающих списков.
    setTextIfEmpty(Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ACCESS_TYPE),
                   Constants::AccessType::toStringList().value(0));
    setTextIfEmpty(Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::MODBUS_DATA_TYPE),
                   Constants::ModBusDataType::toStringList().value(0));

    // Тип данных привода по умолчанию нужен не первый из списка, а IQ24.
    setTextIfEmpty(Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::DRV_DATA_TYPE),
                   Constants::drvDataType::IQ24);

    // Список базовых величин динамический: он берется из соседней таблицы,
    // поэтому здесь вручную выбираем первый непустой пункт, кроме строки с плюсом.
    QStringList baseValues;
    if (baseValuesTable) {
        for (int row = 0; row < baseValuesTable->rowCount() - 1; ++row) {
            QTableWidgetItem* baseItem = baseValuesTable->item(row, 0);
            if (baseItem && !baseItem->text().trimmed().isEmpty()) {
                baseValues << baseItem->text();
            }
        }
    }

    // Базовая величина тоже берется первым доступным вариантом из своей таблицы.
    setTextIfEmpty(Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::BASE_VALUE),
                   baseValues.value(0));
}

void TableManager::updateCellsStateByDataType(int rowIndex, QTableWidget* table) {
    if (!table || rowIndex < 0 || rowIndex >= table->rowCount()) return;

    // Проверяем тип строки - обновление состояния ячеек только для COMMON и USER типов
    QTableWidgetItem* rootItem = table->item(rowIndex, 0);
    QString rowType = rootItem ? rootItem->data(Qt::UserRole).toString() : "";

    if (rowType != Constants::ParamType::COMMON && rowType != Constants::ParamType::USER) {
        return; // Для TITLE и других типов не обновляем
    }

    // Получаем индексы нужных колонок
    int colDrvDataType = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::DRV_DATA_TYPE);
    int colCoefficient = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::COEFFICIENT);
    int colBaseValue = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::BASE_VALUE);

    if (colDrvDataType == -1 || colCoefficient == -1 || colBaseValue == -1) return;

    // Получаем тип данных привода
    QTableWidgetItem* drvTypeItem = table->item(rowIndex, colDrvDataType);
    QString drvType = drvTypeItem ? drvTypeItem->text().trimmed() : "";

    // Если тип не задан, выходим
    if (drvType.isEmpty()) return;

    // Получаем ячейки коэффициента и базовой величины
    QTableWidgetItem* coefficientItem = table->item(rowIndex, colCoefficient);
    if (!coefficientItem) {
        coefficientItem = new QTableWidgetItem();
        table->setItem(rowIndex, colCoefficient, coefficientItem);
    }

    QTableWidgetItem* baseValueItem = table->item(rowIndex, colBaseValue);
    if (!baseValueItem) {
        baseValueItem = new QTableWidgetItem();
        table->setItem(rowIndex, colBaseValue, baseValueItem);
    }

    // Получаем цвет для неактивных ячеек из палитры приложения
    QPalette palette = table->palette();
    QColor disabledColor = palette.color(QPalette::Disabled, QPalette::Base);

    // Применяем правила в зависимости от типа данных
    if (Constants::drvDataType::isStandartNum(drvType)) {
        // StandartNum: коэффициент и базовая величина - некликабельны и пусты
        coefficientItem->setText("");
        coefficientItem->setFlags(coefficientItem->flags() & ~Qt::ItemIsEditable);
        coefficientItem->setBackground(QBrush(disabledColor));

        baseValueItem->setText("");
        baseValueItem->setFlags(baseValueItem->flags() & ~Qt::ItemIsEditable);
        baseValueItem->setBackground(QBrush(disabledColor));

    } else if (Constants::drvDataType::isFloat(drvType)) {
        // float: базовая величина - некликабельна, коэффициент - кликабелен
        coefficientItem->setFlags(coefficientItem->flags() | Qt::ItemIsEditable);
        coefficientItem->setBackground(QBrush()); // Обычный фон

        baseValueItem->setText("");
        baseValueItem->setFlags(baseValueItem->flags() & ~Qt::ItemIsEditable);
        baseValueItem->setBackground(QBrush(disabledColor));

    } else if (Constants::drvDataType::isIQ(drvType)) {
        // IQ тип
        if (drvType == Constants::drvDataType::IQ0) {
            // IQ0: коэффициент - некликабелен, базовая величина - кликабельна
            coefficientItem->setText("");
            coefficientItem->setFlags(coefficientItem->flags() & ~Qt::ItemIsEditable);
            coefficientItem->setBackground(QBrush(disabledColor));

            baseValueItem->setFlags(baseValueItem->flags() | Qt::ItemIsEditable);
            baseValueItem->setBackground(QBrush());
        } else {
            // Остальные IQ: все кликабельно
            coefficientItem->setFlags(coefficientItem->flags() | Qt::ItemIsEditable);
            coefficientItem->setBackground(QBrush());

            baseValueItem->setFlags(baseValueItem->flags() | Qt::ItemIsEditable);
            baseValueItem->setBackground(QBrush());
        }
    }
}

void TableManager::handleCellClicked(QTableWidget* table, int row, int col) {
    // Ctrl+ЛКМ по параметру - копирование с инкрементом адреса,
    // по заголовку - копирование группы вместе с параметрами
    if ((QApplication::keyboardModifiers() & Qt::ControlModifier) && isDataRow(table, row)) {
        if (table == dataTable && rowTypeOf(table, row) == Constants::ParamType::TITLE) {
            copyGroup(table, row);
        } else {
            copyRow(table, row, table == dataTable);
        }
        return;
    }

    if (row == table->rowCount() - 1) {
        // Клик по последней строке - вставляем новую ПЕРЕД ней
        table->insertRow(row);
        setRowType(Constants::ParamType::COMMON, row, table);
        emit dataModified();
        return;
    }

    // По умолчанию QTableWidget открывает редактор ячейки не с первого клика,
    // поэтому для колонок с комбобоксами явно запускаем редактирование сами.
    // Дальше делегат ComboBoxDelegate/DynamicComboBoxDelegate создаст QComboBox
    // и сразу раскроет его список через showPopup().
    bool isComboColumn = false;

    if (table == dataTable) {
        // В основной таблице выпадающие списки стоят не подряд, поэтому
        // определяем их по заголовкам, а не по "магическим" номерам колонок.
        // Так код переживет перестановку колонок в Constants::TableHeaders.
        const QStringList dataHeaders = Constants::TableHeaders::DATA_TABLE();
        isComboColumn =
            col == dataHeaders.indexOf(Constants::TableHeaders::ACCESS_TYPE) ||
            col == dataHeaders.indexOf(Constants::TableHeaders::MODBUS_DATA_TYPE) ||
            col == dataHeaders.indexOf(Constants::TableHeaders::DRV_DATA_TYPE) ||
            col == dataHeaders.indexOf(Constants::TableHeaders::BASE_VALUE) ||
            col == dataHeaders.indexOf(Constants::TableHeaders::NOTE); // Примечание раскрывается по одному клику
    } else if (table == baseValuesTable) {
        // В таблице базовых величин выпадающим списком является столбец "Формат IQ".
        // Здесь пока используем номер колонки напрямую: у таблицы базовых
        // величин нет отдельных констант для имен конкретных колонок.
        isComboColumn = (col == 2) ||
                        col == Constants::TableHeaders::BASE_VALUES_ARR().indexOf("Примечание");
    }

    if (isComboColumn) {
        // Если ячейки еще нет, открывать нечего: делегату нужен существующий
        // QTableWidgetItem, к которому Qt привяжет редактор.
        QTableWidgetItem* clickedItem = table->item(row, col);

        // Запускаем редактор только для редактируемых ячеек, чтобы отключенные поля не раскрывались.
        // Это важно для полей, которые TableManager отключает в зависимости
        // от выбранного типа данных привода.
        if (clickedItem && (clickedItem->flags() & Qt::ItemIsEditable)) {
            // editItem() вызывает назначенный на колонку делегат. Для combo-
            // колонок это создает QComboBox, а уже сам делегат раскрывает список.
            table->editItem(clickedItem);
        }
    }
}

void TableManager::handleCellDoubleClicked(QTableWidget* table, int row, int col) {
    //Не последняя строка
    if (row != table->rowCount() - 1) {

        // Проверяем, что это столбец "Переменная/значение" и тип строки "userType"
        if (col != Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::VARIABLE_VALUE)) return;

        QTableWidgetItem* rootItem = table->item(row, 0); //Скрытая информация текущей строки в 0 ячейке
        QString rowType = rootItem ? rootItem->data(Qt::UserRole).toString() : "";

        if (rowType == Constants::ParamType::USER) {

            QTableWidgetItem* accessTypeItem = table->item(row,  Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ACCESS_TYPE));
            QString accessType = accessTypeItem ? accessTypeItem->text() : ""; //RW или R

            if(!Constants::AccessType::toStringList().contains(accessType)) {
                emit errorOccurred("Неизвестный тип доступа у параметра", "Ошибка редактирования пользовательского кода");
                return;
            }

            int numOfAccessVarieties = Constants::AccessType::isReadOnlyCode(accessType) ? 1 : 2;

            // Получаем текущий пользовательский код
            QVector<QString> userCodeArr;
            for(int i = 0; i < numOfAccessVarieties; i++) {
                QVariant data =  rootItem->data(Qt::UserRole + 1 + i);
                QString userCode = data.isValid() ? data.toString() : "";
                userCodeArr.append(userCode);
            }

            // Создаем и показываем диалог
            UserCodeEditorDialog dialog(accessType, userCodeArr, qobject_cast<QWidget*>(parent()));
            dialog.exec();

            auto result = dialog.getResult();

            if (result.accepted) {
                bool isCodeEmpty = true;

                for(int i = 0; i < result.code.size(); i++) {
                    // Сохраняем код
                    rootItem->setData(Qt::UserRole + 1 + i, result.code[i]);
                    if(!result.code[i].isEmpty()) {
                        isCodeEmpty = false;
                    }
                }

                // Обновляем отображение в ячейке
                QTableWidgetItem* item = table->item(row, col);

                if (isCodeEmpty) {
                    if(item) {
                        item->setText(Constants::UiText::PASTE_CODE);
                        item->setForeground(QBrush());  // Пустая кисть = дефолтный цвет
                    }
                } else {
                    if(item) {
                        item->setText(Constants::UiText::USER_CODE_OK);
                        item->setForeground(QBrush(QColor(0, 140, 0))); // Темно-зеленый цвет
                    }
                }

                emit dataModified();
            }
        }
    }
}

void TableManager::handleTableDataChanged(QTableWidget* table, QTableWidgetItem* item) {
    if (!item) return;

    int row = item->row();
    int col = item->column();

    // Игнорируем последнюю строку (строка с плюсиком)
    if (row == table->rowCount() - 1) return;

    int colAddressDec = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ADDRESS_DEC);
    int colAddressHex = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ADDRESS_HEX);
    int colDrvDataType = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::DRV_DATA_TYPE);

    // Проверяем, что изменился столбец адреса
    if (col == colAddressDec) {
        // Изменился десятичный адрес -> пересчитываем hex
        QString decText = item->text().trimmed();
        if (decText.isEmpty()) return;

        bool ok;
        int decValue = decText.toInt(&ok);

        if (ok) {
            QString hexText = AddressConverter::decToHex(decValue);

            // Временно отключаем сигналы, чтобы избежать рекурсии
            table->blockSignals(true);

            QTableWidgetItem* hexItem = table->item(row, colAddressHex);
            if (!hexItem) {
                hexItem = new QTableWidgetItem();
                table->setItem(row, colAddressHex, hexItem);
            }
            hexItem->setText(hexText);

            table->blockSignals(false);
        }
    }
    else if (col == colAddressHex) {
        // Изменился hex адрес -> пересчитываем десятичный
        QString hexText = item->text().trimmed();
        if (hexText.isEmpty()) return;

        int decValue = AddressConverter::hexToDec(hexText);

        if (decValue != -1) {
            QString decText = QString::number(decValue);

            // Временно отключаем сигналы, чтобы избежать рекурсии
            table->blockSignals(true);

            QTableWidgetItem* decItem = table->item(row, colAddressDec);
            if (!decItem) {
                decItem = new QTableWidgetItem();
                table->setItem(row, colAddressDec, decItem);
            }
            decItem->setText(decText);

            // Обновляем hex с правильным форматом (добавляем 0x)
            QString formattedHex = AddressConverter::decToHex(decValue);
            item->setText(formattedHex);

            table->blockSignals(false);
        }
    }
    else if (col == colDrvDataType) {
        // Изменился тип данных привода -> обновляем состояние ячеек
        if (table == dataTable) {
            updateCellsStateByDataType(row, table);
        }
    }

    // Адрес изменился - переставляем строку на свое место внутри группы.
    // Откладываем до возврата в цикл событий: сейчас мы внутри itemChanged
    // и редактор ячейки еще может работать с этим item.
    if (table == dataTable && (col == colAddressDec || col == colAddressHex)) {
        QPointer<QTableWidget> tablePtr(table);
        QTimer::singleShot(0, this, [this, tablePtr, row, col]() {
            if (tablePtr) sortGroupByAddress(tablePtr, tablePtr->item(row, col));
        });
    }

    // Устанавливаем флаг изменений
    emit dataModified();
}

void TableManager::sortGroupByAddress(QTableWidget* table, QTableWidgetItem* changedItem) {
    if (!table || !changedItem) return;

    const int changedRow = table->row(changedItem);
    const int changedCol = table->column(changedItem);
    if (!isDataRow(table, changedRow)) return;
    if (rowTypeOf(table, changedRow) == Constants::ParamType::TITLE) return;

    // Границы группы: от строки после заголовка до следующего заголовка (или строки с плюсиком)
    int start = changedRow;
    while (start > 0 && rowTypeOf(table, start - 1) != Constants::ParamType::TITLE) {
        start--;
    }
    int end = changedRow + 1;
    while (isDataRow(table, end) && rowTypeOf(table, end) != Constants::ParamType::TITLE) {
        end++;
    }
    if (end - start < 2) return;

    const int colAddressDec = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ADDRESS_DEC);
    const int colCount = table->columnCount();

    // Снимаем строки группы целиком: QTableWidgetItem хранит текст, флаги, цвета
    // и скрытые данные (тип строки, пользовательский код), так что все переезжает вместе
    struct RowData {
        QVector<QTableWidgetItem*> items;
        int address;
        bool hasAddress;
    };
    QVector<RowData> rows;
    for (int row = start; row < end; row++) {
        RowData rowData;
        QTableWidgetItem* decItem = table->item(row, colAddressDec);
        rowData.address = decItem ? decItem->text().trimmed().toInt(&rowData.hasAddress) : 0;
        if (!decItem) rowData.hasAddress = false;
        for (int col = 0; col < colCount; col++) {
            rowData.items << table->item(row, col);
        }
        rows << rowData;
    }

    // Строки без адреса оставляем в конце группы, равные адреса - в прежнем порядке
    std::vector<RowData> sorted(rows.begin(), rows.end());
    std::stable_sort(sorted.begin(), sorted.end(), [](const RowData& a, const RowData& b) {
        if (a.hasAddress != b.hasAddress) return a.hasAddress;
        return a.hasAddress && a.address < b.address;
    });

    bool changed = false;
    for (int i = 0; i < rows.size(); i++) {
        if (sorted[i].items != rows[i].items) {
            changed = true;
            break;
        }
    }
    if (!changed) return;

    const int currentCol = table->currentColumn();

    table->blockSignals(true);
    for (int row = start; row < end; row++) {
        for (int col = 0; col < colCount; col++) {
            table->takeItem(row, col);
        }
    }
    for (int i = 0; i < static_cast<int>(sorted.size()); i++) {
        for (int col = 0; col < colCount; col++) {
            if (sorted[i].items[col]) {
                table->setItem(start + i, col, sorted[i].items[col]);
            }
        }
    }
    table->blockSignals(false);

    // Выделение переезжает вместе с измененной строкой
    const int newRow = table->row(changedItem);
    table->setCurrentCell(newRow, currentCol >= 0 ? currentCol : changedCol);
    table->scrollToItem(changedItem);
}

void TableManager::handleContextMenu(QTableWidget* table, const QPoint &pos) {
    showContextMenuForTable(pos, table);
}

void TableManager::showContextMenuForTable(const QPoint &pos, QTableWidget* table) {
    int row = table->rowAt(pos.y());
    if (!isDataRow(table, row)) return; // Не показываем меню для строки с плюсиком и пустого места

    // Сохраняем номер строки и таблицу для использования в других методах
    contextMenuClickRow = row;
    contextMenuActiveTable = table;

    const QString rowType = rowTypeOf(table, row);
    const bool isTitle = (table == dataTable && rowType == Constants::ParamType::TITLE);

    QMenu contextMenu;

    // Копирование
    QAction *copyAction = contextMenu.addAction("Копировать");
    copyAction->setShortcut(QKeySequence("Ctrl+D"));

    QAction *copyIncAction = nullptr;
    QAction *copyGroupAction = nullptr;
    if (table == dataTable) {
        copyIncAction = contextMenu.addAction("Копировать с инкрементом адреса (Ctrl+ЛКМ)");
        copyIncAction->setShortcut(QKeySequence("Ctrl+Shift+D"));
        copyIncAction->setEnabled(!isTitle);

        copyGroupAction = contextMenu.addAction("Копировать группу с параметрами");
        copyGroupAction->setEnabled(isTitle);
    }

    contextMenu.addSeparator();

    QAction *cutAction = contextMenu.addAction("Вырезать");
    cutAction->setShortcut(QKeySequence::Cut);
    QAction *pasteAction = contextMenu.addAction("Вставить (над строкой)");
    pasteAction->setShortcut(QKeySequence::Paste);
    pasteAction->setEnabled(canPaste(table));

    contextMenu.addSeparator();

    QAction *addAction = contextMenu.addAction("Добавить");
    addAction->setShortcut(QKeySequence(Qt::Key_Insert));
    QAction *deleteAction = contextMenu.addAction("Удалить");
    deleteAction->setShortcut(QKeySequence::Delete);

    QAction *renumberAction = nullptr;
    QAction *commonType = nullptr;
    QAction *userType = nullptr;
    QAction *titleType = nullptr;

    if (table == dataTable) {// Если основная таблица с данными
        renumberAction = contextMenu.addAction("Перенумеровать адреса с...");
        renumberAction->setEnabled(isTitle);

        contextMenu.addSeparator();

        QMenu *subMenu = contextMenu.addMenu("Тип параметра");
        commonType = subMenu->addAction("Обычный");
        userType = subMenu->addAction("Пользовательский");
        titleType = subMenu->addAction("Заголовок");
    }

    QAction *selectedAction = contextMenu.exec(table->mapToGlobal(pos));
    if (!selectedAction) return;

    if (selectedAction == copyAction) {
        copyRow(table, row, false);
    } else if (copyIncAction && selectedAction == copyIncAction) {
        copyRow(table, row, true);
    } else if (copyGroupAction && selectedAction == copyGroupAction) {
        copyGroup(table, row);
    } else if (selectedAction == cutAction) {
        cutRow(table, row);
    } else if (selectedAction == pasteAction) {
        pasteRows(table, row);
    } else if (selectedAction == deleteAction) {
        deleteRow(table, row);
    } else if (selectedAction == addAction) {
        addRow(table, row);
    } else if (renumberAction && selectedAction == renumberAction) {
        renumberAddresses(row);
    } else if (commonType && selectedAction == commonType) {
        setRowType(Constants::ParamType::COMMON, row, table);
        emit dataModified();
    } else if (userType && selectedAction == userType) {
        setRowType(Constants::ParamType::USER, row, table);
        emit dataModified();
    } else if (titleType && selectedAction == titleType) {
        setRowType(Constants::ParamType::TITLE, row, table);
        emit dataModified();
    }
}

void TableManager::setupShortcuts(QTableWidget* table) {
    // Qt::WidgetShortcut - клавиши работают, только когда фокус на самой таблице,
    // а не в открытом редакторе ячейки (иначе Del в редакторе удалил бы строку).
    auto addShortcut = [this, table](const QKeySequence& key, std::function<void(int)> handler) {
        QShortcut* shortcut = new QShortcut(key, table);
        shortcut->setContext(Qt::WidgetShortcut);
        connect(shortcut, &QShortcut::activated, this, [table, handler]() {
            handler(table->currentRow());
        });
    };

    addShortcut(QKeySequence::Delete, [this, table](int row) { deleteRow(table, row); });
    addShortcut(QKeySequence(Qt::Key_Insert), [this, table](int row) { addRow(table, row); });
    addShortcut(QKeySequence("Ctrl+D"), [this, table](int row) { copyRow(table, row, false); });
    addShortcut(QKeySequence::Cut, [this, table](int row) { cutRow(table, row); });
    addShortcut(QKeySequence::Paste, [this, table](int row) { pasteRows(table, row); });

    if (table == dataTable) {
        addShortcut(QKeySequence("Ctrl+Shift+D"), [this, table](int row) {
            if (rowTypeOf(table, row) == Constants::ParamType::TITLE) {
                copyGroup(table, row);
            } else {
                copyRow(table, row, true);
            }
        });
    }
}

bool TableManager::isDataRow(QTableWidget* table, int row) const {
    return table && row >= 0 && row < table->rowCount() - 1;
}

QString TableManager::rowTypeOf(QTableWidget* table, int row) const {
    if (!table) return "";
    QTableWidgetItem* rootItem = table->item(row, 0); //Скрытая информация строки в 0 ячейке
    return rootItem ? rootItem->data(Qt::UserRole).toString() : "";
}

int TableManager::groupEndRow(QTableWidget* table, int titleRow) const {
    // Группа заканчивается на следующем заголовке или на строке с плюсиком
    int row = titleRow + 1;
    while (isDataRow(table, row) && rowTypeOf(table, row) != Constants::ParamType::TITLE) {
        row++;
    }
    return row;
}

void TableManager::cloneRow(QTableWidget* table, int srcRow, int dstRow) {
    table->insertRow(dstRow);
    if (srcRow >= dstRow) srcRow++; // Исходная строка сдвинулась вниз после вставки

    // clone() копирует текст, флаги, цвета и скрытые данные (тип строки, пользовательский код)
    for (int col = 0; col < table->columnCount(); col++) {
        QTableWidgetItem* srcItem = table->item(srcRow, col);
        if (srcItem) {
            table->setItem(dstRow, col, srcItem->clone());
        }
    }

    // Заголовок занимает всю строку
    if (table->columnSpan(srcRow, 0) > 1) {
        table->setSpan(dstRow, 0, 1, table->columnCount());
    }
}

void TableManager::shiftAddress(QTableWidget* table, int row, int delta) {
    int colAddressDec = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ADDRESS_DEC);
    int colAddressHex = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ADDRESS_HEX);
    if (colAddressDec == -1 || colAddressHex == -1) return;

    QTableWidgetItem* decItem = table->item(row, colAddressDec);
    if (!decItem) return;

    bool ok;
    int address = decItem->text().trimmed().toInt(&ok);
    if (!ok) return; // Адрес не задан - не трогаем

    int newAddress = address + delta;
    decItem->setText(QString::number(newAddress));

    QTableWidgetItem* hexItem = table->item(row, colAddressHex);
    if (!hexItem) {
        hexItem = new QTableWidgetItem();
        table->setItem(row, colAddressHex, hexItem);
    }
    hexItem->setText(AddressConverter::decToHex(newAddress));
}

void TableManager::copyRow(QTableWidget* table, int row, bool incrementAddress) {
    if (!isDataRow(table, row)) return;

    // Блокируем сигналы, чтобы itemChanged не срабатывал на каждую скопированную ячейку
    table->blockSignals(true);
    int newRow = row + 1;
    cloneRow(table, row, newRow);
    if (incrementAddress && table == dataTable) {
        shiftAddress(table, newRow, 1);
    }
    table->blockSignals(false);

    table->setCurrentCell(newRow, table->currentColumn() >= 0 ? table->currentColumn() : 0);
    emit dataModified();
}

void TableManager::copyGroup(QTableWidget* table, int titleRow) {
    if (table != dataTable || !isDataRow(table, titleRow)) return;
    if (rowTypeOf(table, titleRow) != Constants::ParamType::TITLE) return;

    int endRow = groupEndRow(table, titleRow);
    int groupSize = endRow - titleRow;

    // Сдвиг адресов: копия группы начинается сразу после последнего адреса исходной
    int colAddressDec = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ADDRESS_DEC);
    int minAddress = INT_MAX;
    int maxAddress = INT_MIN;
    for (int row = titleRow + 1; row < endRow; row++) {
        QTableWidgetItem* decItem = table->item(row, colAddressDec);
        bool ok = false;
        int address = decItem ? decItem->text().trimmed().toInt(&ok) : 0;
        if (ok) {
            minAddress = qMin(minAddress, address);
            maxAddress = qMax(maxAddress, address);
        }
    }
    int delta = (minAddress <= maxAddress) ? (maxAddress - minAddress + 1) : 0;

    table->blockSignals(true);
    for (int i = 0; i < groupSize; i++) {
        cloneRow(table, titleRow + i, endRow + i);
        if (delta != 0) {
            shiftAddress(table, endRow + i, delta);
        }
    }
    table->blockSignals(false);

    table->setCurrentCell(endRow, 0);
    emit dataModified();
}

void TableManager::deleteRow(QTableWidget* table, int row) {
    if (!isDataRow(table, row)) return;

    // Для заголовка с параметрами спрашиваем, что именно удалять
    if (table == dataTable && rowTypeOf(table, row) == Constants::ParamType::TITLE
        && groupEndRow(table, row) > row + 1) {
        QMessageBox msgBox(qobject_cast<QWidget*>(parent()));
        msgBox.setWindowTitle("Удаление заголовка");
        msgBox.setIcon(QMessageBox::Question);
        msgBox.setText("Удалить вместе с параметрами группы?");

        QPushButton* withParamsButton = msgBox.addButton("Да, с параметрами", QMessageBox::YesRole);
        QPushButton* titleOnlyButton = msgBox.addButton("Нет, только заголовок", QMessageBox::NoRole);
        msgBox.addButton("Отмена", QMessageBox::RejectRole);
        msgBox.setDefaultButton(titleOnlyButton);
        msgBox.exec();

        if (msgBox.clickedButton() == withParamsButton) {
            deleteTitleWithParams(table, row);
            return;
        } else if (msgBox.clickedButton() != titleOnlyButton) {
            return; // Отмена
        }
    }

    table->removeRow(row);
    emit dataModified();
}

void TableManager::addRow(QTableWidget* table, int row) {
    if (!table) return;

    // Добавляем строку после той, на которую кликнули,
    // иначе - перед последней строкой (строкой с плюсиком)
    int rowIndex = isDataRow(table, row) ? row + 1 : table->rowCount() - 1;
    table->insertRow(rowIndex);

    //Если основная таблица с данными, то определим дефолтный тип строки
    if (table == dataTable) {
        setRowType(Constants::ParamType::COMMON, rowIndex, table);
    }

    emit dataModified();
}

void TableManager::deleteTitleWithParams(QTableWidget* table, int titleRow) {
    if (!isDataRow(table, titleRow)) return;

    // Удаляем заголовок и все строки до следующего заголовка
    int endRow = groupEndRow(table, titleRow);
    for (int row = endRow - 1; row >= titleRow; row--) {
        table->removeRow(row);
    }

    emit dataModified();
}

void TableManager::renumberAddresses(int titleRow) {
    if (!contextMenuActiveTable || titleRow < 0) return;

    // Создаем простой диалог
    QDialog dialog(qobject_cast<QWidget*>(parent()));
    dialog.setWindowTitle("Перенумеровать адреса");

    // Создаем layout
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Добавляем label
    QLabel *label = new QLabel("Начальный адрес:", &dialog);
    layout->addWidget(label);

    // Добавляем SpinBox для ввода числа
    QSpinBox *spinBox = new QSpinBox(&dialog);
    spinBox->setMinimum(0);
    spinBox->setMaximum(65535);
    spinBox->setValue(0);
    layout->addWidget(spinBox);

    // Создаем кнопки с нужными текстами
    QDialogButtonBox *buttonBox = new QDialogButtonBox(&dialog);
    buttonBox->addButton("Перенумеровать", QDialogButtonBox::AcceptRole);
    buttonBox->addButton("Отменить", QDialogButtonBox::RejectRole);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addWidget(buttonBox);

    // Показываем диалог
    if (dialog.exec() != QDialog::Accepted) {
        return; // Пользователь отменил ввод
    }

    int startAddress = spinBox->value();

    // Последняя строка - это строка с плюсиком, поэтому предпоследняя - это rowCount - 2
    int lastRow = contextMenuActiveTable->rowCount() - 2;
    int currentAddress = startAddress;

    // Получаем индексы колонок с адресами
    int colAddressDec = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ADDRESS_DEC);
    int colAddressHex = Constants::TableHeaders::DATA_TABLE().indexOf(Constants::TableHeaders::ADDRESS_HEX);

    if (colAddressDec == -1 || colAddressHex == -1) return;

    // Начинаем со следующей строки после заголовка
    for (int row = titleRow + 1; row <= lastRow; row++) {
        QTableWidgetItem* rootItem = contextMenuActiveTable->item(row, 0);
        QString rowType = rootItem ? rootItem->data(Qt::UserRole).toString() : "";

        // Если встретили новый TITLE, останавливаемся
        if (rowType == Constants::ParamType::TITLE) {
            break;
        }

        // Перенумеровываем только COMMON и USER параметры
        if (rowType == Constants::ParamType::COMMON || rowType == Constants::ParamType::USER) {
            // Блокируем сигналы чтобы избежать лишних пересчетов
            contextMenuActiveTable->blockSignals(true);

            // Устанавливаем новый адрес (десятичный)
            QTableWidgetItem* decItem = contextMenuActiveTable->item(row, colAddressDec);
            if (!decItem) {
                decItem = new QTableWidgetItem();
                contextMenuActiveTable->setItem(row, colAddressDec, decItem);
            }
            decItem->setText(QString::number(currentAddress));

            // Устанавливаем новый адрес (hex)
            QString hexText = AddressConverter::decToHex(currentAddress);
            QTableWidgetItem* hexItem = contextMenuActiveTable->item(row, colAddressHex);
            if (!hexItem) {
                hexItem = new QTableWidgetItem();
                contextMenuActiveTable->setItem(row, colAddressHex, hexItem);
            }
            hexItem->setText(hexText);

            contextMenuActiveTable->blockSignals(false);

            // Увеличиваем адрес для следующего параметра
            currentAddress++;
        }
    }

    emit dataModified();
}
