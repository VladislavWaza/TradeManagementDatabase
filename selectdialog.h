#ifndef SELECTDIALOG_H
#define SELECTDIALOG_H

#include <QDialog>
#include <QSqlTableModel>

namespace Ui {
class SelectDialog;
}


/*Класс нужен для выбора строк из таблицы, без возможности её изменения
  Создается окно с таблицей, в которой нужно выбрать запись кликнув по ней 2 раза*/
class SelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectDialog(QSqlTableModel*& model, QWidget *parent = nullptr);
    ~SelectDialog();

private slots:
    //Слот обработки двойного клика по записи
    void on_tableView_doubleClicked(const QModelIndex &index);

private:
    Ui::SelectDialog *m_ui;

signals:
    //Сигнал испускающийся при двойном клике по записи и передающий её номер
    void selected(int row);
};

#endif // SELECTDIALOG_H
