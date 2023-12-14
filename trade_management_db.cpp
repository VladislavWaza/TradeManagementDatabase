#include "trade_management_db.h"

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
    if (table_type == TableType::Products)
        return "products";
    if (table_type == TableType::DepartmentProducts)
        return "department_products";
    if (table_type == TableType::BaseProducts)
        return "base_products";
    emit errorMsg(QString("[tableTypeToTableName] Неизвестный тип таблицы!"));
    return QString();
}

void TradeManagementDB::getModel(QSqlTableModel *&model)
{
    if (m_table_type == TableType::None)
    {
        emit errorMsg(QString("[getModel] Не выбрана активная таблица!"));
        return;
    }

    if (model != nullptr)
    {
        disconnect(model, &QSqlTableModel::beforeUpdate, this, &TradeManagementDB::onUpdate);
        delete model;
    }
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
    else if (m_table_type == TableType::Products)
        addRowToProducts();
    else if (m_table_type == TableType::DepartmentProducts)
        addRowToDepartmentProducts();
    else if (m_table_type == TableType::BaseProducts)
        addRowToBaseProducts();
    else
        emit errorMsg(QString("[addRow] Неизвестный тип таблицы!"));
}

void TradeManagementDB::addTables()
{
    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        //Создание таблицы оптовых баз
        if (!query.exec("CREATE TABLE IF NOT EXISTS wholesale_bases ("
                        "id INT PRIMARY KEY,"
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
                        "id INT PRIMARY KEY,"
                        "name VARCHAR(255) NOT NULL CHECK (LENGTH(name) >= 3),"
                        "class VARCHAR(255) NOT NULL CHECK (LENGTH(class) >= 3),"
                        "actual_address VARCHAR(255) NOT NULL CHECK (LENGTH(actual_address) >= 7),"
                        "ogrn VARCHAR(15) NOT NULL check(ogrn not like '%[^0-9]%' AND (LENGTH(ogrn) = 15 OR LENGTH(ogrn) = 13)),"
                        "inn VARCHAR(12) NOT NULL check(inn not like '%[^0-9]%' AND LENGTH(inn) = 12),"
                        "kpp VARCHAR(9) NOT NULL check(kpp not like '%[^0-9]%' AND LENGTH(kpp) = 9),"
                        "base_id INT NOT NULL,"
                        "FOREIGN KEY (base_id) REFERENCES wholesale_bases(id));"))
        {
            qDebug() << "[addTables]" << query.lastError().text();
        }
        //Создание таблицы товаров базы
        if (!query.exec("CREATE TABLE IF NOT EXISTS base_products ("
                        "base_id INT NOT NULL,"
                        "article VARCHAR(50) NOT NULL CHECK (LENGTH(article) >= 3),"
                        "name VARCHAR(255) NOT NULL CHECK (LENGTH(name) >= 3),"
                        "type VARCHAR(255) NOT NULL CHECK (LENGTH(type) >= 3),"
                        "price DECIMAL(10, 2) NOT NULL CHECK (price >= 0),"
                        "count INT NOT NULL CHECK (count >= 0),"
                        "PRIMARY KEY (base_id, article),"
                        "FOREIGN KEY (base_id) REFERENCES wholesale_bases(id)"
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

void TradeManagementDB::addRowToShops()
{
    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        if (!query.exec("SELECT COUNT(id) from shops;"))
        {
            emit errorMsg("[addRowToShops] " + query.lastError().text());
        }
        int count = 0;
        while (query.next())
            count = query.value(0).toInt();
        query.prepare("INSERT INTO shops (id, name, class, actual_address, ogrn, inn, kpp, base_id)"
                      "VALUES (:new_id, 'Название магазина', 'Класс магазина', 'г. Москва',"
                      "'123456789012345', '123456789012', '123456789', 1);");
        query.bindValue(":new_id", count + 1);

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
        if (!query.exec("SELECT COUNT(id) from wholesale_bases;"))
        {
            emit errorMsg("[addRowToWholesaleBases] " + query.lastError().text());
        }
        int count = 0;
        while (query.next())
            count = query.value(0).toInt();
        query.prepare("INSERT INTO wholesale_bases (id, name, actual_address, ogrn, inn, kpp)"
                      "VALUES (:new_id, 'Название базы', 'г. Москва',"
                      "'123456789012345', '123456789012', '123456789');");
        query.bindValue(":new_id", count + 1);

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

}

void TradeManagementDB::addRowToProducts()
{

}

void TradeManagementDB::addRowToDepartmentProducts()
{

}

void TradeManagementDB::addRowToBaseProducts()
{
    if (m_db.transaction())
    {
        QSqlQuery query(m_db);
        if (!query.exec("SELECT COUNT(id) from base_products;"))
        {
            emit errorMsg("[addRowToBaseProducts] " + query.lastError().text());
        }
        int count = 0;
        while (query.next())
            count = query.value(0).toInt();
        query.prepare("INSERT INTO base_products (base_id, article, name, type, price, count)"
                      "VALUES (:base_id, '000', 'Именование товара', "
                      "'Тип товара', '0.00', '0');");
        query.bindValue(":base_id", count + 1);

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

void TradeManagementDB::onUpdate(int row, QSqlRecord &record)
{
    qDebug() << row << record.count() << record.field(0).value() << record.field(1).value();
    record.setGenerated(0, false);
    qDebug() << record.field(0).tableName();
    if (m_db.transaction())
    {
        qDebug() << m_db.databaseName();
        QSqlQuery query(m_db);
        query.prepare("UPDATE shops SET"
                               "name = :name"
                               "MCC = :MCC");
        query.bindValue(":name", record.field(1).value().toString());
        query.bindValue(":MCC", record.field(1).value().toInt());

        if(!query.exec())
        {
            qDebug() << query.lastError().text();
        }
        if (m_db.commit())
            qDebug() << "good";
    }
}
