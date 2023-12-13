#ifndef TRADEMANAGEMENTDB_H
#define TRADEMANAGEMENTDB_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlTableModel>

class TradeManagementDB : public QObject
{
    Q_OBJECT
public:
    enum class TableType {None, Shops, WholesaleBases, Departments, Products, DepartmentProducts, BaseProducts};

    explicit TradeManagementDB(QObject *parent = nullptr);
    bool isOpen() const;
    bool isValid() const;
    QString lastErrorText() const;

    void bind(const TableType& table_type);
    void unbind();

    void getModel(QSqlTableModel*& model);
    void addRow();
private:
    QSqlDatabase m_db;
    TableType m_table_type = TableType::None;

    void addTables();
    QString tableTypeToTableName(const TableType& table_type);

    void addRowToShops();
    void addRowToWholesaleBases();
    void addRowToDepartments();
    void addRowToProducts();
    void addRowToDepartmentProducts();
    void addRowToBaseProducts();

signals:
    void errorMsg(const QString& msg);

public slots:
    void onUpdate(int row, QSqlRecord &record);
};

#endif // TRADEMANAGEMENTDB_H
