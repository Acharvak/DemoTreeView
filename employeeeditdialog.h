#ifndef EMPLOYEEEDITDIALOG_H
#define EMPLOYEEEDITDIALOG_H

#include "controller.h"
#include "model.h"

#include <QDialog>

namespace Ui {
class EmployeeEditDialog;
}

using namespace TreeDemo;

class EmployeeEditDialog : public QDialog {
    Q_OBJECT

  public:
    explicit EmployeeEditDialog(QWidget* parent, Controller& controller,
                                QSharedPointer<Department> department,
                                QSharedPointer<const Employee> employee);
    ~EmployeeEditDialog();

  private slots:
    void saveButtonClicked(bool checked);
    void cancelButtonClicked(bool checked);
    void textChanged(const QString& text);

  private:
    Ui::EmployeeEditDialog* ui;
    Controller& controller;
    QSharedPointer<Department> department;
    QSharedPointer<const Employee> employee;
    QString initialSurname, initialName, initialMiddleName, initialFunction,
        initialSalary;
};

#endif // EMPLOYEEEDITDIALOG_H
