#include "authorization.h"
#include "ui_authorization.h"

Authorization::Authorization(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::Authorization)
{
    m_ui->setupUi(this);
}

Authorization::~Authorization()
{
    delete m_ui;
}

void Authorization::on_enter_clicked()
{
    emit authorization(m_ui->login->text(), m_ui->pass->text());
}

