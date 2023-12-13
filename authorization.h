#ifndef AUTHORIZATION_H
#define AUTHORIZATION_H

#include <QDialog>

namespace Ui {
class Authorization;
}

class Authorization : public QDialog
{
    Q_OBJECT

public:
    explicit Authorization(QWidget *parent = nullptr);
    ~Authorization();
signals:
    void authorization(const QString &login, const QString &pass);

private slots:
    void on_enter_clicked();

private:
    Ui::Authorization *m_ui;
};

#endif // AUTHORIZATION_H
