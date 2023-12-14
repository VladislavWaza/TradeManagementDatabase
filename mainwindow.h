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
    void on_showShops_triggered();
    void onDatabaseError(const QString& msg);
    void on_addRow_clicked();
    void on_save_clicked();
    void on_update_clicked();

    void on_showBases_triggered();

    void on_showBasesProds_triggered();

private:
    void changeAccessRights();


    enum class UserType {Unauthorized, Admin};

    Ui::MainWindow *m_ui;
    TradeManagementDB m_db;
    UserType m_user_type = UserType::Unauthorized;
    QSqlTableModel* m_model = nullptr;
};
#endif // MAINWINDOW_H
