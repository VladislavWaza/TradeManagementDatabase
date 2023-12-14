#include "selectdialog.h"
#include "ui_selectdialog.h"

SelectDialog::SelectDialog(QSqlTableModel*& model, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SelectDialog)
{
    m_ui->setupUi(this);
    m_ui->tableView->setModel(model);
    m_ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

SelectDialog::~SelectDialog()
{
    delete m_ui;
}

void SelectDialog::on_tableView_doubleClicked(const QModelIndex &index)
{
    emit selected(index.row());
}

