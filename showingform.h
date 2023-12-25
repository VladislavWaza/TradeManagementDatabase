#ifndef SHOWINGFORM_H
#define SHOWINGFORM_H

#include <QWidget>
#include <QSqlTableModel>

namespace Ui {
class ShowingForm;
}

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
