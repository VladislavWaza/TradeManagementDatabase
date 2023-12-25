#include "trade_management_db.h"
#include "selectdialog.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlField>
#include <QDebug>

TradeManagementDB::TradeManagementDB(QObject *parent)
    : QObject{parent}
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("TradeManagement.db");
    m_db.open();
    QSqlQuery query;
    query.exec("PRAGMA foreign_keys = ON;");
    addTables();
}

bool TradeManagementDB::isOpen() const
{
    return m_db.isOpen();
}

bool TradeManagementDB::isValid() const
{
    return m_db.isValid();
}

QString TradeManagementDB::lastErrorText() const
{
    return m_db.lastError().text();
}

void TradeManagementDB::bind(const TableType& table_type)
{
    m_table_type = table_type;
}

void TradeManagementDB::unbind()
{
    m_table_type = TableType::None;
}

QString TradeManagementDB::tableTypeToTableName(const TableType &table_type)
{
    if (table_type == TableType::None)
    {
        emit errorMsg(QString("[tableTypeToTableName] Не выбрана активная таблица!"));
        return QString();
    }
    if (table_type == TableType::Shops)
        return "shops";
    if (table_type == TableType::WholesaleBases)
        return "wholesale_bases";
    if (table_type == TableType::Departments)
        return "departments";
    if (table_type == TableType::ShopProducts)
        return "shop_products";
    if (table_type == TableType::BaseProducts)
        return "base_products";
    if (table_type == TableType::DepartmentProducts)
        return "department_products";
    if (table_type == TableType::Temp)
        return "temp_table";
    emit errorMsg(QString("[tableTypeToTableName] Неизвестный тип таблицы!"));
    return QString();
}

QSqlRecord TradeManagementDB::recordFromSelectDialog(const TableType &table_type, const QString& title)
{
    //Создаем модель
    QSqlTableModel* model = new QSqlTableModel(nullptr, m_db);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setTable(tableTypeToTableName(table_type));
    model->select();

    //Создаем и запускаем окно
    SelectDialog* select_dialog = new SelectDialog(model);
    if (title != QString())
        select_dialog->setWindowTitle(title);
    connect(select_dialog, &SelectDialog::selected, this, &TradeManagementDB::onSelected);
    select_dialog->exec();
    disconnect(select_dialog, &SelectDialog::selected, this, &TradeManagementDB::onSelected);

    int row = m_selected_row;
    //Проверяем результат на валидность
    if (m_selected_row == -1)
    {
        emit errorMsg("[rowFromSelectDialog] Строка не выбрана!");
        delete select_dialog;
        delete model;
        return QSqlRecord();
    }
    //Делаем его снова невалидным
    m_selected_row = -1;
    QSqlRecord res = model->record(row);
    delete select_dialog;
    delete model;
    return res;
}

void TradeManagementDB::addRow()
{
    if (m_table_type == TableType::None)
    {
        emit errorMsg(QString("[addRow] Не выбрана активная таблица!"));
    }
    else if (m_table_type == TableType::Shops)
        addRowToShops();
    else if (m_table_type == TableType::WholesaleBases)
        addRowToWholesaleBases();
    else if (m_table_type == TableType::Departments)
        addRowToDepartments();
    else if (m_table_type == TableType::ShopProducts)
        addRowToShopProducts();
    else if (m_table_type == TableType::BaseProducts)
        addRowToBaseProducts();
    else if (m_table_type == TableType::DepartmentProducts)
        addRowToDepartmentProducts();
    else
        emit errorMsg(QString("[addRow] Неизвестный тип таблицы!"));
}

void TradeManagementDB::deleteRow()
{
    QSqlRecord record = recordFromSelectDialog(m_table_type, "Выберите удаляемую запись");
    if (record == QSqlRecord())
        return;
    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        QString query_text = "DELETE FROM " + tableTypeToTableName(m_table_type) + " WHERE ";
        for (int i = 0; i < record.count(); ++i)
        {
            query_text.append(record.fieldName(i) + "='" + record.value(i).toString() + "'");
            if (i != record.count() - 1)
                query_text.append(" AND ");
        }
        query_text.append(";");

        if (!query.exec(query_text))
        {
            emit errorMsg("[deleteRow] " + query.lastError().text());
        }

        if (m_db.commit())
            return;
        else
            emit errorMsg("[deleteRow] commit failed");
    }
    else
        emit errorMsg("[deleteRow] transaction failed");
}

//Обработка различных запросов
void TradeManagementDB::showProds(const TableType &table_type, QSqlTableModel *&model)
{
    if (table_type == TableType::Shops || table_type == TableType::Departments || table_type == TableType::WholesaleBases)
    {
        QSqlRecord record = recordFromSelectDialog(table_type, "Выберите запись");
        if (record == QSqlRecord())
            return;

        if (m_db.transaction())
        {
            QSqlQuery query(m_db);
            query.exec("DROP TABLE prods_info;");
            QString query_text;

            if (table_type == TableType::Shops)
                query_text = "CREATE TABLE prods_info AS SELECT * "
                             "FROM " + tableTypeToTableName(TableType::ShopProducts) + " "
                             "WHERE shop_id = " + record.value("id").toString() + ';';

            if (table_type == TableType::WholesaleBases)
                query_text = "CREATE TABLE prods_info AS SELECT * "
                             "FROM " + tableTypeToTableName(TableType::BaseProducts) + " "
                             "WHERE base_id = " + record.value("id").toString() + ';';

            if (table_type == TableType::Departments)
                query_text = "CREATE TABLE prods_info AS "
                             "SELECT dp.shop_id, dp.department_id, dp.article, sp.name, "
                             "sp.prod_type, sp.variety, sp.price, dp.quantity "
                             "FROM " + tableTypeToTableName(TableType::DepartmentProducts) + " dp "
                             "JOIN " + tableTypeToTableName(TableType::ShopProducts) + " sp "
                             "ON sp.shop_id = dp.shop_id AND sp.article = dp.article "
                             "WHERE dp.shop_id = " + record.value("shop_id").toString() + " "
                             "AND dp.department_id = " + record.value("id").toString() + ';';
            if (!query.exec(query_text))
            {
                emit errorMsg("[showProds] " + query.lastError().text());
            }
            else
            {
                //Создаем модель
                model = new QSqlTableModel(nullptr, m_db);
                model->setEditStrategy(QSqlTableModel::OnManualSubmit);
                model->setTable("prods_info");
                model->select();
                //Деструктор модели надо вызывать отдельно
            }
            if (m_db.commit())
                return;
            else
                emit errorMsg("[showProds] commit failed");
        }
        else
            emit errorMsg("[showProds] transaction failed");

    }
    else
        emit errorMsg("[showProds] Не корректный тип таблицы!");
}

void TradeManagementDB::showShopProds(QSqlTableModel *&model)
{
    QSqlRecord record = recordFromSelectDialog(TableType::Shops, "Выберите магазин");
    if (record == QSqlRecord())
        return;

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        query.exec("DROP TABLE prods_info;");
        QString query_text = "CREATE TABLE prods_info AS "
                            "SELECT sp.shop_id, sh.name AS shop_name, dp.department_id, d.name AS dep_name, "
                            "sp.article, sp.name, sp.prod_type, sp.variety, sp.price, dp.quantity "
                            "FROM " + tableTypeToTableName(TableType::ShopProducts) + " sp "
                            "JOIN " + tableTypeToTableName(TableType::DepartmentProducts) + " dp "
                            "ON sp.shop_id = dp.shop_id AND sp.article = dp.article "
                            "JOIN " + tableTypeToTableName(TableType::Shops) + " sh "
                            "ON sp.shop_id = sh.id AND dp.shop_id = sh.id "
                            "JOIN " + tableTypeToTableName(TableType::Departments) + " d "
                            "ON d.shop_id = sh.id AND d.id = dp.department_id "
                            "WHERE dp.shop_id = " + record.value("id").toString() + " "
                            "GROUP BY sp.shop_id, shop_name, dp.department_id, dep_name, sp.article, sp.name, "
                            "sp.prod_type, sp.variety, sp.price, dp.quantity;";
        if (!query.exec(query_text))
        {
            emit errorMsg("[showShopProds] " + query.lastError().text());
        }
        else
        {
            //Создаем модель
            model = new QSqlTableModel(nullptr, m_db);
            model->setEditStrategy(QSqlTableModel::OnManualSubmit);
            model->setTable("prods_info");
            model->select();
            //Деструктор модели надо вызывать отдельно
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[showShopProds] commit failed");
    }
    else
        emit errorMsg("[showShopProds] transaction failed");
}

void TradeManagementDB::showIdenticalProds(QSqlTableModel *&model)
{
    QSqlRecord record = recordFromSelectDialog(TableType::Shops, "Выберите магазин");
    if (record == QSqlRecord())
        return;

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        query.exec("DROP TABLE prods_info;");

        QString query_text = "CREATE TABLE prods_info AS "
                            "SELECT sp.shop_id, sh.name AS shop_name, dp.department_id, d.name AS dep_name, "
                            "sp.article, sp.name, sp.prod_type, sp.variety, sp.price, dp.quantity "
                            "FROM " + tableTypeToTableName(TableType::ShopProducts) + " sp "
                            "JOIN ( "
                            "SELECT article, shop_id FROM " + tableTypeToTableName(TableType::DepartmentProducts) + " "
                            "WHERE shop_id = " + record.value("id").toString() + " "
                            "GROUP BY article "
                            "HAVING COUNT(DISTINCT department_id) > 1 ) sub "
                            "ON sp.shop_id = sub.shop_id AND sp.article = sub.article "
                            "JOIN " + tableTypeToTableName(TableType::DepartmentProducts) + " dp "
                            "ON sp.shop_id = dp.shop_id AND sp.article = dp.article "
                            "JOIN " + tableTypeToTableName(TableType::Shops) + " sh "
                            "ON sp.shop_id = sh.id AND dp.shop_id = sh.id "
                            "JOIN " + tableTypeToTableName(TableType::Departments) + " d "
                            "ON d.shop_id = sh.id AND d.id = dp.department_id "
                            "WHERE dp.shop_id = " + record.value("id").toString() + " "
                            "GROUP BY sp.shop_id, dp.department_id, sp.article, sp.name, sp.prod_type, "
                            "sp.variety, sp.price, dp.quantity;";

        if (!query.exec(query_text))
        {
            emit errorMsg("[showIdenticalProds] " + query.lastError().text());
        }
        else
        {
            //Создаем модель
            model = new QSqlTableModel(nullptr, m_db);
            model->setEditStrategy(QSqlTableModel::OnManualSubmit);
            model->setTable("prods_info");
            model->select();
            //Деструктор модели надо вызывать отдельно
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[showIdenticalProds] commit failed");
    }
    else
        emit errorMsg("[showIdenticalProds] transaction failed");
}

void TradeManagementDB::showManagers(QSqlTableModel *&model)
{
    QSqlRecord record = recordFromSelectDialog(TableType::Shops, "Выберите магазин");
    if (record == QSqlRecord())
        return;

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        query.exec("DROP TABLE info_table;");

        QString query_text = "CREATE TABLE info_table AS "
                            "SELECT sh.id AS shop_id, sh.name AS shop_name, d.id AS department_id, "
                            "d.name AS department_name, d.manager "
                            "FROM " + tableTypeToTableName(TableType::Shops) + " sh "
                            "JOIN " + tableTypeToTableName(TableType::Departments) + " d "
                            "ON d.shop_id = sh.id "
                            "WHERE d.shop_id = " + record.value("id").toString() + " "
                            "GROUP BY shop_id, shop_name, department_id, department_name, d.manager;";

        if (!query.exec(query_text))
        {
            emit errorMsg("[showManagers] " + query.lastError().text());
        }
        else
        {
            //Создаем модель
            model = new QSqlTableModel(nullptr, m_db);
            model->setEditStrategy(QSqlTableModel::OnManualSubmit);
            model->setTable("info_table");
            model->select();
            //Деструктор модели надо вызывать отдельно
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[showManagers] commit failed");
    }
    else
        emit errorMsg("[showManagers] transaction failed");
}

void TradeManagementDB::showMissingProds(QSqlTableModel *&model)
{
    QSqlRecord record = recordFromSelectDialog(TableType::Shops, "Выберите магазин");
    if (record == QSqlRecord())
        return;

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        query.exec("DROP TABLE prods_info;");
        QString query_text = "CREATE TABLE prods_info AS "
                            "SELECT sp.shop_id, sh.name AS shop_name, dp.department_id, d.name AS dep_name, "
                            "sp.article, sp.name, sp.prod_type, sp.variety, sp.price, dp.quantity "
                            "FROM " + tableTypeToTableName(TableType::ShopProducts) + " sp "
                            "JOIN " + tableTypeToTableName(TableType::DepartmentProducts) + " dp "
                            "ON sp.shop_id = dp.shop_id AND sp.article = dp.article "
                            "JOIN " + tableTypeToTableName(TableType::Shops) + " sh "
                            "ON sp.shop_id = sh.id AND dp.shop_id = sh.id "
                            "JOIN " + tableTypeToTableName(TableType::Departments) + " d "
                            "ON d.shop_id = sh.id AND d.id = dp.department_id "
                            "WHERE dp.shop_id = " + record.value("id").toString() + " AND dp.quantity = 0 "
                            "GROUP BY sp.shop_id, shop_name, dp.department_id, dep_name, sp.article, sp.name, "
                            "sp.prod_type, sp.variety, sp.price, dp.quantity;";

        if (!query.exec(query_text))
        {
            emit errorMsg("[showMissingProds] " + query.lastError().text());
        }
        else
        {
            //Создаем модель
            model = new QSqlTableModel(nullptr, m_db);
            model->setEditStrategy(QSqlTableModel::OnManualSubmit);
            model->setTable("prods_info");
            model->select();
            //Деструктор модели надо вызывать отдельно
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[showMissingProds] commit failed");
    }
    else
        emit errorMsg("[showMissingProds] transaction failed");
}

void TradeManagementDB::closeDepartment()
{
    QSqlRecord record = recordFromSelectDialog(TableType::Departments, "Выберите отдел, которые хотите закрыть");
    if (record == QSqlRecord())
        return;
    int shop_id = record.value("shop_id").toInt();
    int closed_id = record.value("id").toInt();
    if (m_db.transaction())
    {
        //Составляем таблицу отделов этого магазина, кроме удаляемого отдела
        QSqlQuery query(m_db);
        query.exec("DROP TABLE temp_table;");
        QString query_text = "CREATE TABLE temp_table AS "
                            "SELECT * "
                            "FROM " + tableTypeToTableName(TableType::Departments) + " "
                            "WHERE shop_id = " + QString::number(shop_id) + " "
                            "AND id != " + QString::number(closed_id) + ";";
        if (!query.exec(query_text))
        {
            emit errorMsg("[closeDepartment] " + query.lastError().text());
        }
        record = recordFromSelectDialog(TableType::Temp, "Выберите отдел, в который хотите передать товары");
        if (record == QSqlRecord())
            return;
        int new_id = record.value("id").toInt();

        //Составляем таблицу из товаров удаляемого отдела
        query.exec("DROP TABLE temp_table;");
        query_text = "CREATE TABLE temp_table AS "
                     "SELECT quantity AS quantity2, department_id AS department_id2, article AS article2 "
                     "FROM " + tableTypeToTableName(TableType::DepartmentProducts) + " "
                     "WHERE department_id = " + QString::number(closed_id) + " "
                     "AND shop_id = " + QString::number(shop_id) + ";";
        if (!query.exec(query_text))
        {
            emit errorMsg("[closeDepartment] " + query.lastError().text());
        }

        //Переносим дублирующиеся товары
        query_text = "UPDATE " + tableTypeToTableName(TableType::DepartmentProducts) + " "
                     "SET quantity = quantity + dp1.quantity2 "
                     "FROM " + tableTypeToTableName(TableType::Temp) + " AS dp1 "
                     "WHERE article = dp1.article2 "
                     "AND department_id = " + QString::number(new_id) + " "
                     "AND dp1.department_id2 = " + QString::number(closed_id) + " "
                     "AND shop_id = " + QString::number(shop_id) + ";";
        if (!query.exec(query_text))
        {
            emit errorMsg("[closeDepartment] " + query.lastError().text());
        }

        //Удаляем записи товаров из удаляемого отдела, которые уже есть в отделе-приёмнике
        query_text = "DELETE FROM " + tableTypeToTableName(TableType::DepartmentProducts) + " "
                     "WHERE department_id = " + QString::number(closed_id) + " "
                     "AND shop_id = " + QString::number(shop_id) + " "
                     "AND article IN (SELECT DISTINCT article "
                     "FROM " + tableTypeToTableName(TableType::DepartmentProducts) + " "
                     "WHERE department_id = " + QString::number(new_id) + " "
                     "AND shop_id = " + QString::number(shop_id) + ");";
        if (!query.exec(query_text))
        {
            emit errorMsg("[closeDepartment] " + query.lastError().text());
        }

        //Перенесем оставшиеся товары
        query.exec("DROP TABLE temp_table;");
        query_text = "UPDATE " + tableTypeToTableName(TableType::DepartmentProducts) + " "
                     "SET department_id = " + QString::number(new_id) + " "
                     "WHERE department_id = " + QString::number(closed_id) + " "
                     "AND shop_id = " + QString::number(shop_id) + ";";
        if (!query.exec(query_text))
        {
            emit errorMsg("[closeDepartment] " + query.lastError().text());
        }

        //Удалим отдел
        query_text = "DELETE FROM " + tableTypeToTableName(TableType::Departments) + " "
                     "WHERE id = " + QString::number(closed_id) + " "
                     "AND shop_id = " + QString::number(shop_id) + ";";
        if (!query.exec(query_text))
        {
            emit errorMsg("[closeDepartment] " + query.lastError().text());
        }

        if (m_db.commit())
            return;
        else
            emit errorMsg("[closeDepartment] commit failed");
    }
    else
        emit errorMsg("[closeDepartment] transaction failed");
}

//Выдача модели активной таблицы с возможностью изменения
void TradeManagementDB::getModel(QSqlTableModel *&model)
{
    //Проверяем выбрана ли активная таблица
    if (m_table_type == TableType::None)
    {
        emit errorMsg(QString("[getModel] Не выбрана активная таблица!"));
        return;
    }

    //Удаляем старую модель
    if (model != nullptr)
    {
        disconnect(model, &QSqlTableModel::beforeUpdate, this, &TradeManagementDB::onUpdate);
        delete model;
    }
    //Создаем новую и соединяем её со слотом валидации
    model = new QSqlTableModel(nullptr, m_db);
    connect(model, &QSqlTableModel::beforeUpdate, this, &TradeManagementDB::onUpdate);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    QString table_name = tableTypeToTableName(m_table_type);
    if (!table_name.isEmpty())
    {
        model->setTable(table_name);
        model->select();
    }
}

//Создание таблиц при их отсутсвии
void TradeManagementDB::addTables()
{
    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        //Создание таблицы оптовых баз
        if (!query.exec("CREATE TABLE IF NOT EXISTS wholesale_bases ("
                        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                        "name VARCHAR(255) NOT NULL CHECK (LENGTH(name) >= 3),"
                        "actual_address VARCHAR(255) NOT NULL CHECK (LENGTH(actual_address) >= 7),"
                        "ogrn VARCHAR(15) NOT NULL check(ogrn not like '%[^0-9]%' AND (LENGTH(ogrn) = 15 OR LENGTH(ogrn) = 13)),"
                        "inn VARCHAR(12) NOT NULL check(inn not like '%[^0-9]%' AND LENGTH(inn) = 12),"
                        "kpp VARCHAR(9) NOT NULL check(kpp not like '%[^0-9]%' AND LENGTH(kpp) = 9));"))
        {
            qDebug() << "[addTables]" << query.lastError().text();
        }
        //Создание таблицы магазинов
        if (!query.exec("CREATE TABLE IF NOT EXISTS shops ("
                        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                        "name VARCHAR(255) NOT NULL CHECK (LENGTH(name) >= 3),"
                        "class VARCHAR(255) NOT NULL CHECK (LENGTH(class) >= 3),"
                        "actual_address VARCHAR(255) NOT NULL CHECK (LENGTH(actual_address) >= 7),"
                        "ogrn VARCHAR(15) NOT NULL check(ogrn not like '%[^0-9]%' AND (LENGTH(ogrn) = 15 OR LENGTH(ogrn) = 13)),"
                        "inn VARCHAR(12) NOT NULL check(inn not like '%[^0-9]%' AND LENGTH(inn) = 12),"
                        "kpp VARCHAR(9) NOT NULL check(kpp not like '%[^0-9]%' AND LENGTH(kpp) = 9),"
                        "base_id INTEGER NOT NULL,"
                        "FOREIGN KEY (base_id) REFERENCES wholesale_bases(id) ON UPDATE CASCADE ON DELETE CASCADE);"))
        {
            qDebug() << "[addTables]" << query.lastError().text();
        }
        //Создание таблицы товаров базы
        if (!query.exec("CREATE TABLE IF NOT EXISTS base_products ("
                        "base_id INTEGER NOT NULL,"
                        "article VARCHAR(50) NOT NULL CHECK (LENGTH(article) >= 1),"
                        "name VARCHAR(255) NOT NULL CHECK (LENGTH(name) >= 3),"
                        "prod_type VARCHAR(255) NOT NULL CHECK (LENGTH(prod_type) >= 3),"
                        "variety VARCHAR(50) NOT NULL CHECK (LENGTH(variety) >= 1),"
                        "price DECIMAL(10, 2) NOT NULL CHECK (price >= 0),"
                        "quantity INTEGER NOT NULL CHECK (quantity >= 0),"
                        "PRIMARY KEY (base_id, article),"
                        "FOREIGN KEY (base_id) REFERENCES wholesale_bases(id) ON UPDATE CASCADE ON DELETE CASCADE"
                        ");"))
        {
            qDebug() << "[addTables]" << query.lastError().text();
        }

        //Создаение таблицы отделов
        if (!query.exec("CREATE TABLE IF NOT EXISTS departments ("
                        "id INTEGER NOT NULL,"
                        "shop_id INTEGER NOT NULL,"
                        "name VARCHAR(255) NOT NULL CHECK (LENGTH(name) >= 3),"
                        "manager VARCHAR(255) NOT NULL CHECK (LENGTH(manager) >= 5),"
                        "PRIMARY KEY (id, shop_id),"
                        "FOREIGN KEY (shop_id) REFERENCES shops(id) ON UPDATE CASCADE ON DELETE CASCADE"
                        ");"))
        {
            qDebug() << "[addTables]" << query.lastError().text();
        }

        //Создание таблицы товаров магазина
        if (!query.exec("CREATE TABLE IF NOT EXISTS shop_products ("
                        "shop_id INTEGER NOT NULL,"
                        "article VARCHAR(50) NOT NULL CHECK (LENGTH(article) >= 1),"
                        "name VARCHAR(255) NOT NULL CHECK (LENGTH(name) >= 3),"
                        "prod_type VARCHAR(255) NOT NULL CHECK (LENGTH(prod_type) >= 3),"
                        "variety VARCHAR(50) NOT NULL CHECK (LENGTH(variety) >= 1),"
                        "price DECIMAL(10, 2) NOT NULL CHECK (price >= 0),"
                        "PRIMARY KEY (shop_id, article),"
                        "FOREIGN KEY (shop_id) REFERENCES shops(id) ON UPDATE CASCADE ON DELETE CASCADE"
                        ");"))
        {
            qDebug() << "[addTables]" << query.lastError().text();
        }

        //Создание таблицы товаров отдела
        if (!query.exec("CREATE TABLE IF NOT EXISTS department_products ("
                        "shop_id INTEGER NOT NULL,"
                        "article VARCHAR(50) NOT NULL CHECK (LENGTH(article) >= 1),"
                        "department_id INTEGER NOT NULL,"
                        "quantity INTEGER NOT NULL CHECK (quantity >= 0),"
                        "PRIMARY KEY (shop_id, article, department_id),"
                        "FOREIGN KEY (shop_id, article) REFERENCES shop_products (shop_id, article) ON UPDATE CASCADE ON DELETE CASCADE,"
                        "FOREIGN KEY (shop_id) REFERENCES shops (id) ON UPDATE CASCADE ON DELETE CASCADE,"
                        "FOREIGN KEY (department_id, shop_id) REFERENCES departments (id, shop_id) ON UPDATE CASCADE ON DELETE CASCADE"
                        ");"))
        {
            qDebug() << "[addTables]" << query.lastError().text();
        }

        if (m_db.commit())
            return;
        else
            qDebug() << "[addTables] commit failed";
    }
    else
        qDebug() << "[addTables] transaction failed";
}

//Создаение новых строк
void TradeManagementDB::addRowToShops()
{
    //Принимаем связанную с магазином базу
    int base_id = 0;
    QSqlRecord record = recordFromSelectDialog(TableType::WholesaleBases);
    if (record == QSqlRecord())
        return;
    else
        base_id = record.value("id").toInt();

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO shops (name, class, actual_address, ogrn, inn, kpp, base_id)"
                      "VALUES ('Название магазина', 'Класс магазина', 'г. Москва',"
                      "'123456789012345', '123456789012', '123456789', :base_id);");
        query.bindValue(":base_id", base_id);

        if (!query.exec())
        {
            emit errorMsg("[addRowToShops] " + query.lastError().text());
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[addRowToShops] commit failed");
    }
    else
        emit errorMsg("[addRowToShops] transaction failed");
}

void TradeManagementDB::addRowToWholesaleBases()
{
    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        query.prepare("INSERT INTO wholesale_bases (name, actual_address, ogrn, inn, kpp)"
                      "VALUES ('Название базы', 'г. Москва',"
                      "'123456789012345', '123456789012', '123456789');");
        if (!query.exec())
        {
            emit errorMsg("[addRowToWholesaleBases] " + query.lastError().text());
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[addRowToWholesaleBases] commit failed");
    }
    else
        emit errorMsg("[addRowToWholesaleBases] transaction failed");
}

void TradeManagementDB::addRowToDepartments()
{
    //Принимаем связанный с отделом магазин
    int shop_id = 0;
    QSqlRecord record = recordFromSelectDialog(TableType::Shops);
    if (record == QSqlRecord())
        return;
    else
        shop_id = record.value("id").toInt();

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        //Находим максимальный номер отдела в этом магазине
        query.prepare("SELECT MAX(id) FROM departments WHERE (shop_id = :shop_id)");
        query.bindValue(":shop_id", shop_id);
        if (!query.exec())
        {
            emit errorMsg("[addRowToDepartments] " + query.lastError().text());
        }
        int count = 0;
        while (query.next())
            count = query.value(0).toInt();

        //Добавлем новую запись
        query.prepare("INSERT INTO departments (id, shop_id, name, manager)"
                      "VALUES (:id, :shop_id, 'Именование отдела', 'Заведующий');");
        query.bindValue(":shop_id", shop_id);
        query.bindValue(":id", count + 1);

        if (!query.exec())
        {
            emit errorMsg("[addRowToDepartments] " + query.lastError().text());
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[addRowToDepartments] commit failed");
    }
    else
        emit errorMsg("[addRowToDepartments] transaction failed");
}

void TradeManagementDB::addRowToShopProducts()
{
    //Принимаем связанный с товаром магазин
    int shop_id = 0;
    QSqlRecord record = recordFromSelectDialog(TableType::Shops);
    if (record == QSqlRecord())
        return;
    else
        shop_id = record.value("id").toInt();

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        //Проверяем наличие такой записи
        query.prepare("SELECT COUNT(*) FROM shop_products WHERE (shop_id = :shop_id AND article = '000')");
        query.bindValue(":shop_id", shop_id);
        if (!query.exec())
        {
            emit errorMsg("[addRowToShopProducts] " + query.lastError().text());
        }
        int tmp = 0;
        while (query.next())
            tmp = query.value(0).toInt();
        if (tmp != 0)
        {
            emit errorMsg("[addRowToShopProducts] Уже есть товар в это магазине и с артикулом '000'");
        }

        //Добавлем новую запись
        query.prepare("INSERT INTO shop_products (shop_id, article, name, prod_type, price, variety) "
                      "VALUES (:shop_id, '000', 'Именование товара', 'Тип товара', 0.00, '0');");
        query.bindValue(":shop_id", shop_id);

        if (!query.exec())
        {
            emit errorMsg("[addRowToShopProducts] " + query.lastError().text());
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[addRowToShopProducts] commit failed");
    }
    else
        emit errorMsg("[addRowToShopProducts] transaction failed");
}

void TradeManagementDB::addRowToBaseProducts()
{
    //Принимаем связанную с товаром базу
    int base_id = 0;
    QSqlRecord record = recordFromSelectDialog(TableType::WholesaleBases);
    if (record == QSqlRecord())
        return;
    else
        base_id = record.value("id").toInt();

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        //Проверяем наличие такой записи
        query.prepare("SELECT COUNT(*) FROM base_products WHERE (base_id = :base_id AND article = '000')");
        query.bindValue(":base_id", base_id);
        if (!query.exec())
        {
            emit errorMsg("[addRowToBaseProducts] " + query.lastError().text());
        }
        int tmp = 0;
        while (query.next())
            tmp = query.value(0).toInt();
        if (tmp != 0)
        {
            emit errorMsg("[addRowToBaseProducts] Уже есть товар с этой базы и артикулом '000'");
        }

        //Добавлем новую запись
        query.prepare("INSERT INTO base_products (base_id, article, name, prod_type, price, quantity, variety)"
                      "VALUES (:base_id, '000', 'Именование товара', "
                      "'Тип товара', 0.00, 0, '0');");
        query.bindValue(":base_id", base_id);

        if (!query.exec())
        {
            emit errorMsg("[addRowToBaseProducts] " + query.lastError().text());
        }
        if (m_db.commit())
            return;
        else
            emit errorMsg("[addRowToBaseProducts] commit failed");
    }
    else
        emit errorMsg("[addRowToBaseProducts] transaction failed");
}

void TradeManagementDB::addRowToDepartmentProducts()
{
    //Принимаем отдел
    int dep_id = 0;
    QSqlRecord record = recordFromSelectDialog(TableType::Departments);
    if (record == QSqlRecord())
        return;
    else
        dep_id = record.value("id").toInt();


    int shop_id = record.value("shop_id").toInt();
    //Принимаем товар
    QString article;
    record = recordFromSelectDialog(TableType::ShopProducts);
    if (record == QSqlRecord())
        return;
    else
    {
        //Проверяем что выбранный товар и отдел относятся к одному магазину
        if (shop_id != record.value("shop_id").toInt())
        {
            emit errorMsg("[addRowToDepartmentProducts] Выбранные товар и отдел не относятся к одному магазину!");
        }
        else
            article = record.value("article").toString();
    }

    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        //Проверяем наличие такой записи
        query.prepare("SELECT COUNT(*) FROM department_products "
                      "WHERE (shop_id = :shop_id AND article = :article AND department_id = :dep_id)");
        query.bindValue(":shop_id", shop_id);
        query.bindValue(":article", article);
        query.bindValue(":dep_id", dep_id);
        if (!query.exec())
        {
            emit errorMsg("[addRowToDepartmentProducts] " + query.lastError().text());
        }
        int tmp = 0;
        while (query.next())
            tmp = query.value(0).toInt();
        if (tmp != 0)
        {
            emit errorMsg("[addRowToDepartmentProducts] Уже есть товар с этого отдела и магазина и с таким артикулом");
        }

        //Добавлем новую запись
        query.prepare("INSERT INTO department_products (shop_id, article, department_id, quantity)"
                      "VALUES (:shop_id, :article, :department_id, '0');");
        query.bindValue(":shop_id", shop_id);
        query.bindValue(":article", article);
        query.bindValue(":department_id", dep_id);

        if (!query.exec())
        {
            emit errorMsg("[addRowToDepartmentProducts] " + query.lastError().text());
        }

        if (m_db.commit())
            return;
        else
            emit errorMsg("[addRowToDepartmentProducts] commit failed");
    }
    else
        emit errorMsg("[addRowToDepartmentProducts] transaction failed");
}

void TradeManagementDB::onUpdate(int row, QSqlRecord &record)
{
    for (int i = 0; i < record.count(); ++i)
    {
        if (record.field(i).name().contains("id") || record.field(i).name().contains("article"))
        {
            //record.setGenerated(i, false);
        }
    }
}

void TradeManagementDB::onSelected(int row)
{
    m_selected_row = row;
    static_cast<SelectDialog*>(sender())->close();
}
