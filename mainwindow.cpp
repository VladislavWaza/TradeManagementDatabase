#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "authorization.h"
#include "showingform.h"

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

void MainWindow::createShowingForm(QSqlTableModel *&model, const QString& title)
{
    ShowingForm* showing_form = new ShowingForm(model);
    showing_form->setWindowTitle(title);
    showing_form->show();
}

void MainWindow::onDatabaseError(const QString &msg)
{
    QMessageBox::critical(this, QString("ОШИБКА!"), msg);
}

/*Функции показа таблиц*/

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


void MainWindow::on_showDepartments_triggered()
{
    m_db.bind(TradeManagementDB::TableType::Departments);
    m_db.getModel(m_model);
    m_ui->tableView->setModel(m_model);
}

void MainWindow::on_showShopProds_triggered()
{
    m_db.bind(TradeManagementDB::TableType::ShopProducts);
    m_db.getModel(m_model);
    m_ui->tableView->setModel(m_model);
}

void MainWindow::on_showDepProds_triggered()
{
    m_db.bind(TradeManagementDB::TableType::DepartmentProducts);
    m_db.getModel(m_model);
    m_ui->tableView->setModel(m_model);
}

/*Функции добвления/удаления строки, обновления, сохранения таблицы*/

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
            QMessageBox::critical(this, QString("Невалидные изменения!"), m_model->lastError().text());
        }
}

void MainWindow::on_update_clicked()
{
    m_db.getModel(m_model);
    m_ui->tableView->setModel(m_model);
}

void MainWindow::on_deleteRow_clicked()
{
    if (QMessageBox::question(this, "Хотите удалить запись?", "Это также удалит все зависимые записи при их наличии", "Удалить", "Отмена") == 0)
    {
        m_db.deleteRow();
        m_db.getModel(m_model);
        m_ui->tableView->setModel(m_model);
    }
}

/*Показ товаров*/

void MainWindow::on_prodsOnBase_triggered()
{
    QSqlTableModel* model = nullptr;
    m_db.showProds(TradeManagementDB::TableType::WholesaleBases, model);
    createShowingForm(model, "Информация о товарах на базе");
}

void MainWindow::on_prodsOnShop_triggered()
{
    QSqlTableModel* model = nullptr;
    m_db.showProds(TradeManagementDB::TableType::Shops, model);
    createShowingForm(model, "Информация о товарах в магазине");
}

void MainWindow::on_prodsOnDep_triggered()
{
    QSqlTableModel* model = nullptr;
    m_db.showProds(TradeManagementDB::TableType::Departments, model);
    createShowingForm(model, "Информация о товарах в отделе");
}


void MainWindow::on_prodsOnShopDeps_triggered()
{
    QSqlTableModel* model = nullptr;
    m_db.showShopProds(model);
    createShowingForm(model, "Информация о товарах во всех отделах магазина");
}


void MainWindow::on_identicalProds_triggered()
{
    QSqlTableModel* model = nullptr;
    m_db.showIdenticalProds(model);
    createShowingForm(model, "Информация о одинаковых товарах в магазине");
}


void MainWindow::on_managers_triggered()
{
    QSqlTableModel* model = nullptr;
    m_db.showManagers(model);
    createShowingForm(model, "Информация о заведующих отделами магазина");
}


void MainWindow::on_missingProds_triggered()
{
    QSqlTableModel* model = nullptr;
    m_db.showMissingProds(model);
    createShowingForm(model, "Информация о отсутствующих товарах в магазине, которые можно заказать на базе");
}

void MainWindow::on_closeDep_triggered()
{
    if (QMessageBox::question(this, "Вы действительно хотите закрыть отдел?", "Товары будут переданы в другой отдел", "Да", "Нет") == 0)
    {
        m_db.closeDepartment();
    }
}

