#include "tablemanager.h"
#include "usercodeeditordialog.h"
#include "addressconverter.h"

TableManager::TableManager(QObject *parent, QTableWidget* dataTable, QTableWidget* baseValuesTable)
    : QObject(parent),
    dataTable(dataTable),
    baseValuesTable(baseValuesTable)
{
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
    item->setForeground(Qt::green);
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

    int rowCounter = 0;
    for(const QJsonValue &val : std::as_const(data)) {

        QJsonObject obj = val.toObject();

        // Добавляем строку в таблицу
        table->insertRow(rowCounter);

        //Получаем строку из объекта json и кладем в соответсвующую колонку
        for(int col = 0; col < mainKeys.size(); col++) {
            QString key = mainKeys[col];
            QString str = obj[key].toString();
            table->setItem(rowCounter, col, new QTableWidgetItem(str));
        }

        if(configRow) { //Конфигурируем строки
            QString paramType = obj[Constants::JsonKeys::Data::PARAM_TYPE].toString();

            if(paramType == Constants::ParamType::COMMON) { //Встяавляем в соответсвующую колонку название перемеенной
                QString varName = obj[Constants::JsonKeys::Data::VAR_NAME].toString();
                QVector<QString> data = {varName};
                setRowType(paramType, rowCounter, table, data);
            } else if(paramType == Constants::ParamType::USER) {
                //Пользовательская ячейка. Выставляем PASTE YOUR CODE в ячейке, если пустая: USER CODE
                QString userCode_R = obj[Constants::JsonKeys::Data::USER_CODE_R].toString();
                QString userCode_W = obj[Constants::JsonKeys::Data::USER_CODE_W].toString();
                QVector<QString> data = {userCode_R, userCode_W};
                QString accessType = obj[Constants::JsonKeys::Data::ACCESS_TYPE].toString();
                setRowType(paramType, rowCounter, table, data, accessType);
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

        // Сбрасываем форматирование первой ячейки
        QFont font = itemRoot->font();
        font.setBold(false);
        font.setPointSize(font.pointSize()); // Оставляем текущий размер
        itemRoot->setFont(font);
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
        QString varName = !data.isEmpty() ? data[0] : "";
        item->setText(varName); //Переменная хранится в 0 элементе
        item->setFlags(item->flags() | Qt::ItemIsEditable); // делаем редактируемым

    } else if (rowType == Constants::ParamType::USER) {
        bool isEmpty = true;
        //В корневой элемент сохраняем пользовательский код
        if(accessType == Constants::AccessType::READ_ONLY) { //Если R, то тогда нам должны были передать только посльзовательский код на чтение
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
            item->setForeground(QBrush(Qt::green));
        }
        item->setFlags(item->flags() & ~Qt::ItemIsEditable); // убираем возможность редактирования

    } else if (rowType == Constants::ParamType::TITLE) {
        // Объединяем все столбцы в одну ячейку
        table->setSpan(rowIndex, 0, 1, table->columnCount());

        // Устанавливаем текст из varName в первую ячейку
        QString titleText = !data.isEmpty() ? data[0] : "";
        itemRoot->setText(titleText);

        // Настраиваем внешний вид
        QFont font = itemRoot->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 1);
        itemRoot->setFont(font);
        itemRoot->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // Делаем редактируемым
        itemRoot->setFlags(itemRoot->flags() | Qt::ItemIsEditable);
    }
}

void TableManager::handleCellClicked(QTableWidget* table, int row, int col) {
    if (row == table->rowCount() - 1) {
        // Клик по последней строке - вставляем новую ПЕРЕД ней
        table->insertRow(row);
        setRowType(Constants::ParamType::COMMON, row, table);
        emit dataModified();
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

            int numOfAccessVarieties = (accessType == Constants::AccessType::READ_ONLY) ? 1 : 2;

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
                        item->setForeground(QBrush(Qt::green));
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

    // Устанавливаем флаг изменений
    emit dataModified();
}

void TableManager::handleContextMenu(QTableWidget* table, const QPoint &pos) {
    showContextMenuForTable(pos, table);
}

void TableManager::showContextMenuForTable(const QPoint &pos, QTableWidget* table) {
    int row = table->rowAt(pos.y());
    if (row < 0) return;

    // Не показывать контекстное меню для последней строки (строки с плюсиком)
    if (row == table->rowCount() - 1) {
        return;
    }

    // Сохраняем номер строки и таблицу для использования в других методах
    contextMenuClickRow = row;
    contextMenuActiveTable = table;

    QMenu contextMenu;

    QAction *deleteAction = contextMenu.addAction("Удалить");
    QAction *addAction = contextMenu.addAction("Добавить");

    QAction *commonType = nullptr;
    QAction *userType = nullptr;
    QAction *titleType = nullptr;


    if(table == dataTable) {// Если основная таблица с данными
        QMenu *subMenu = contextMenu.addMenu("Тип параметра");
        commonType = subMenu->addAction("Обычный");
        userType = subMenu->addAction("Пользовательский");
        titleType = subMenu->addAction("Заголовок");
    }

    QAction *selectedAction = contextMenu.exec(table->mapToGlobal(pos));

    if (selectedAction == deleteAction) {
        deleteRow();
    } else if (selectedAction == addAction) {
        addRow();
    } else if (commonType && selectedAction == commonType) {
        setRowType(Constants::ParamType::COMMON, contextMenuClickRow, contextMenuActiveTable);
        emit dataModified();
    } else if(userType && selectedAction == userType) {
        setRowType(Constants::ParamType::USER, contextMenuClickRow, contextMenuActiveTable);
        emit dataModified();
    } else if(titleType && selectedAction == titleType) {
        setRowType(Constants::ParamType::TITLE, contextMenuClickRow, contextMenuActiveTable);
        emit dataModified();
    }
}

void TableManager::deleteRow() {
    if (!contextMenuActiveTable) return;

    if (contextMenuClickRow >= 0 && contextMenuClickRow < contextMenuActiveTable->rowCount() - 1) {
        contextMenuActiveTable->removeRow(contextMenuClickRow);
        emit dataModified();
    }
}

void TableManager::addRow() {
    if (!contextMenuActiveTable) return;

    int rowIndex = -1;

    if (contextMenuClickRow >= 0) {
        // Добавляем строку после той, на которую кликнули
        rowIndex = contextMenuClickRow + 1;
        contextMenuActiveTable->insertRow(rowIndex);

    } else {
        // Если не знаем где кликнули, добавляем перед последней строкой (строкой с плюсиком)
        rowIndex = contextMenuActiveTable->rowCount() - 1;
        contextMenuActiveTable->insertRow(rowIndex);
    }

    //Если основная таблица с данными, то определим дефолтный тип строки
    if(contextMenuActiveTable == dataTable && rowIndex > -1) {
        setRowType(Constants::ParamType::COMMON, rowIndex, contextMenuActiveTable);
    }

    emit dataModified();
}
