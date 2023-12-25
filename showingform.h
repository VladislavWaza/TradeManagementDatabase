#ifndef SHOWINGFORM_H
#define SHOWINGFORM_H

#include <QWidget>
#include <QSqlTableModel>

namespace Ui {
class ShowingForm;
}

/*Класс нужен для отображения какой-либо таблицы без возможности изменения
Принимает ссылку на указаель на модель таблицы, которую удаляет в деструкторе
Удаляется при закрытии*/
class ShowingForm : public QWidget
{
    Q_OBJECT

public:
    explicit ShowingForm(QSqlTableModel*& model, QWidget *parent = nullptr);
    ~ShowingForm();

private:
    Ui::ShowingForm *m_ui;
    QSqlTableModel* m_model = nullptr;
};

#endif // SHOWINGFORM_H
