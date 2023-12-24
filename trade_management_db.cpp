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
    emit errorMsg(QString("[tableTypeToTableName] Неизвестный тип таблицы!"));
    return QString();
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

int TradeManagementDB::fieldFromSelectDialog(const TableType &table_type, const QString& name)
{
    //Создаем модель
    QSqlTableModel* model = new QSqlTableModel(nullptr, m_db);
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->setTable(tableTypeToTableName(table_type));
    model->select();

    //Создаем и запускаем окно
    SelectDialog* select_dialog = new SelectDialog(model);
    connect(select_dialog, &SelectDialog::selected, this, &TradeManagementDB::onSelected);
    select_dialog->exec();
    disconnect(select_dialog, &SelectDialog::selected, this, &TradeManagementDB::onSelected);

    //Принимаем результат
    int result = m_selected_row;
    //Делаем его снова невалидным
    m_selected_row = -1;
    //Проверяем принятый на валидность
    if (result == -1)
    {
        emit errorMsg("[rowFromSelectDialog] Строка не выбрана!");
        delete select_dialog;
        delete model;
        return -1;
    }
    result = model->record(result).value(name).toInt();
    delete select_dialog;
    delete model;
    return result;
}

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
                        "type VARCHAR(255) NOT NULL CHECK (LENGTH(type) >= 3),"
                        "price DECIMAL(10, 2) NOT NULL CHECK (price >= 0),"
                        "count INTEGER NOT NULL CHECK (count >= 0),"
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
                        "type VARCHAR(255) NOT NULL CHECK (LENGTH(type) >= 3),"
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
                        "count INTEGER NOT NULL CHECK (count >= 0),"
                        "PRIMARY KEY (shop_id, article, department_id),"
                        "FOREIGN KEY (shop_id, article) REFERENCES shop_products(shop_id, article) ON UPDATE CASCADE ON DELETE CASCADE,"
                        "FOREIGN KEY (department_id) REFERENCES departments(id) ON UPDATE CASCADE ON DELETE CASCADE"
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
    //Принимаем связанную с магазином базу
    int base_id = fieldFromSelectDialog(TableType::WholesaleBases, "id");
    if (base_id == -1)
        return;

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
    int shop_id = fieldFromSelectDialog(TableType::Shops, "id");
    if (shop_id == -1)
        return;

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
    int shop_id = fieldFromSelectDialog(TableType::Shops, "id");
    if (shop_id == -1)
        return;

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
        query.prepare("INSERT INTO shop_products (shop_id, article, name, type, price)"
                      "VALUES (:shop_id, '000', 'Именование товара', 'Тип товара', 0.00);");
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
    int base_id = fieldFromSelectDialog(TableType::WholesaleBases, "id");
    if (base_id == -1)
        return;

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
        query.prepare("INSERT INTO base_products (base_id, article, name, type, price, count)"
                      "VALUES (:base_id, '000', 'Именование товара', "
                      "'Тип товара', 0.00, 0);");
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
