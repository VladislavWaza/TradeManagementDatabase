#ifndef SELECTDIALOG_H
#define SELECTDIALOG_H

#include <QDialog>
#include <QSqlTableModel>

namespace Ui {
class SelectDialog;
}

class SelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectDialog(QSqlTableModel*& model, QWidget *parent = nullptr);
    ~SelectDialog();

private slots:
    void on_tableView_doubleClicked(const QModelIndex &index);

private:
    Ui::SelectDialog *m_ui;

signals:
    void selected(int row);
};

#endif // SELECTDIALOG_H
