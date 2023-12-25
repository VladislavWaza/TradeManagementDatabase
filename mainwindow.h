#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "trade_management_db.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onAuthorization(const QString &login, const QString &pass);
private slots:
    //Слот выводящий ошибку которая испускается по сигналу от TradeManagementDB
    void onDatabaseError(const QString& msg);
    //Cлоты обработки нажатий кнопок удаления/добавления строки, обновления/сохранения таблицы
    void on_addRow_clicked();
    void on_deleteRow_clicked();
    void on_save_clicked();
    void on_update_clicked();

    //Слоты показа таблиц
    void on_showShops_triggered();
    void on_showBases_triggered();
    void on_showBasesProds_triggered();
    void on_showDepartments_triggered();
    void on_showShopProds_triggered();
    void on_showDepProds_triggered();

    //Слоты показа информации о товарах
    void on_prodsOnBase_triggered();
    void on_prodsOnShop_triggered();
    void on_prodsOnDep_triggered();
    void on_prodsOnShopDeps_triggered();

private:
    void changeAccessRights();

    void createShowingForm(QSqlTableModel*& model, const QString &title);

    enum class UserType {Unauthorized, Admin};

    Ui::MainWindow *m_ui;
    TradeManagementDB m_db;
    UserType m_user_type = UserType::Unauthorized;
    QSqlTableModel* m_model = nullptr;
};
#endif // MAINWINDOW_H
