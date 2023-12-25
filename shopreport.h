#ifndef SHOPREPORT_H
#define SHOPREPORT_H

#include <QWidget>
#include <QVBoxLayout>

#include "trade_management_db.h"

namespace Ui {
class ShopReport;
}

class ShopReport : public QWidget
{
    Q_OBJECT

public:
    explicit ShopReport(TradeManagementDB& db, QWidget *parent = nullptr);
    ~ShopReport();

private:
    Ui::ShopReport *m_ui;
    QVBoxLayout *m_box_layout;
    QList<QSqlTableModel*> m_models;
};

#endif // SHOPREPORT_H
