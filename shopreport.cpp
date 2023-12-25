#include "shopreport.h"
#include "ui_shopreport.h"

#include <QGroupBox>
#include <QTableView>

ShopReport::ShopReport(TradeManagementDB& db, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::ShopReport),
    m_box_layout(new QVBoxLayout)
{
    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    m_ui->scrollArea->widget()->setLayout(m_box_layout);
    setWindowTitle("Отчет по магазину");
    int shop_id = db.getShopID();
    m_ui->textBrowser->append(db.getShopInfo(shop_id));
    QList<int> deps = db.getShopDepartmentsIDs(shop_id);
    for (int i = 0; i < deps.size(); ++i)
    {
        QLabel *info = new QLabel;
        info->autoFillBackground();
        info->setText(db.getDepartmentInfo(deps[i], shop_id));
        QTableView *table_view = new QTableView;
        QSqlTableModel* model = nullptr;
        db.getDepartmentProducts(model, deps[i], shop_id);
        m_models.append(model);
        table_view->setModel(model);
        table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        QVBoxLayout *vbox = new QVBoxLayout;
        vbox->addWidget(info);
        vbox->addWidget(table_view);
        QGroupBox *row = new QGroupBox(m_ui->scrollArea);
        row->setLayout(vbox);
        row->setFixedHeight(300);
        m_ui->scrollArea->widget()->layout()->addWidget(row);
    }

}

ShopReport::~ShopReport()
{
    for (int i = 0; i < m_models.size(); ++i)
        delete m_models[i];
    delete m_box_layout;
    delete m_ui;
}
