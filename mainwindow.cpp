#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "authorization.h"

#include <QMessageBox>
#include <QSqlError>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
{
    m_ui->setupUi(this);

    Authorization* authorizationWindow = new Authorization;
    connect(authorizationWindow, &Authorization::authorization, this, &MainWindow::onAuthorization);
    authorizationWindow->exec();
    disconnect(authorizationWindow, &Authorization::authorization, this, &MainWindow::onAuthorization);
    delete authorizationWindow;
    if( !m_db.isOpen())
    {
        QMessageBox::critical(this, QString("Не удалось открыть базу данных!"), m_db.lastErrorText());
    }
    connect(&m_db, &TradeManagementDB::errorMsg, this, &MainWindow::onDatabaseError);
}

MainWindow::~MainWindow()
{
    delete m_model;
    delete m_ui;
}

void MainWindow::onAuthorization(const QString &login, const QString &pass)
{
    //if (login == QString("vlad") && pass == QString("ETU is best"))
    {
        m_user_type = UserType::Admin;
        qDebug() << "Admin " << login <<"is logged in!";
        static_cast<Authorization*>(sender())->close();
    }
    //TODO
    changeAccessRights();
}

void MainWindow::changeAccessRights()
{
    //TODO!
}

void MainWindow::on_showShops_triggered()
{
    m_db.bind(TradeManagementDB::TableType::Shops);
    m_db.getModel(m_model);
    m_ui->tableView->setModel(m_model);
}

void MainWindow::on_showBases_triggered()
{
    m_db.bind(TradeManagementDB::TableType::WholesaleBases);
    m_db.getModel(m_model);
    m_ui->tableView->setModel(m_model);
}

void MainWindow::on_showBasesProds_triggered()
{
    m_db.bind(TradeManagementDB::TableType::BaseProducts);
    m_db.getModel(m_model);
    m_ui->tableView->setModel(m_model);
}


void MainWindow::onDatabaseError(const QString &msg)
{
    QMessageBox::critical(this, QString("ОШИБКА!"), msg);
}

void MainWindow::on_addRow_clicked()
{
   m_db.addRow();
   m_db.getModel(m_model);
   m_ui->tableView->setModel(m_model);
}

void MainWindow::on_save_clicked()
{
    if (m_model != nullptr)
        if (!m_model->submitAll())
        {
            QMessageBox::critical(this, QString("Не валидные изменения!"), m_model->lastError().text());
        }
}

void MainWindow::on_update_clicked()
{
    m_db.getModel(m_model);
    m_ui->tableView->setModel(m_model);
}
