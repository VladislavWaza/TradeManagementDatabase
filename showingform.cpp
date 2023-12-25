#include "showingform.h"
#include "ui_showingform.h"

ShowingForm::ShowingForm(QSqlTableModel*& model, QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::ShowingForm)
{
    setAttribute(Qt::WA_DeleteOnClose);
    m_model = model;
    m_ui->setupUi(this);
    m_ui->tableView->setModel(model);
    m_ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

ShowingForm::~ShowingForm()
{
    delete m_model;
    delete m_ui;
}
