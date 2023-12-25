#ifndef TRADEMANAGEMENTDB_H
#define TRADEMANAGEMENTDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlTableModel>

class TradeManagementDB : public QObject
{
    Q_OBJECT
public:
    enum class TableType {None, Shops, WholesaleBases, Departments, ShopProducts, BaseProducts, DepartmentProducts};

    explicit TradeManagementDB(QObject *parent = nullptr);
    //Функции передают результат от БД
    bool isOpen() const;
    bool isValid() const;
    QString lastErrorText() const;

    //Устанавливает активную таблицу
    void bind(const TableType& table_type);
    void unbind();

    //Возвращает модель согласно установленной таблице
    void getModel(QSqlTableModel*& model);
    //Добавляет строку в активную таблицу
    void addRow();
    //Удаляет строку из активной таблицы
    void deleteRow();
    //Выводит информацию о товарах внутри одного магазина/отдела/базы
    void showProds(const TableType& table_type, QSqlTableModel *&model);
private:
    QSqlDatabase m_db;
    TableType m_table_type = TableType::None;
    int m_selected_row = -1;

    //Добавляет все таблицы если они еще не добавлены
    void addTables();
    //Переводит тип таблицы в её имя
    QString tableTypeToTableName(const TableType& table_type);

    //Добавление строки в каждую из таблиц
    void addRowToShops();
    void addRowToWholesaleBases();
    void addRowToDepartments();
    void addRowToShopProducts();
    void addRowToBaseProducts();
    void addRowToDepartmentProducts();

    //Возвращает запись из таблицы table_type которую выбирает пользователь в окне SelectDialog
    QSqlRecord recordFromSelectDialog(const TableType& table_type, const QString& title = QString());
signals:
    void errorMsg(const QString& msg);

public slots:
    //Проверка валидации изменений
    void onUpdate(int row, QSqlRecord &record);
    //Принимает выбранную строку из таблицы
    void onSelected(int row);
};

#endif // TRADEMANAGEMENTDB_H
