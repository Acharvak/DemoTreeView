#ifndef DEPARTMENTEDITDIALOG_H
#define DEPARTMENTEDITDIALOG_H

#include "controller.h"
#include "model.h"

#include <QDialog>

namespace Ui {
class DepartmentEditDialog;
}

using namespace TreeDemo;

class DepartmentEditDialog : public QDialog {
    Q_OBJECT

  public:
    explicit DepartmentEditDialog(
        QWidget* parent, Controller& controller,
        QSharedPointer<Department> parent_department,
        QSharedPointer<Department> department = nullptr);
    ~DepartmentEditDialog();

  private slots:
    void nameChanged(const QString& text);
    void saveButtonClicked(bool checked);
    void cancelButtonClicked(bool checked);

  private:
    Ui::DepartmentEditDialog* ui;
    Controller& controller;
    QSharedPointer<Department> parentDepartment;
    QSharedPointer<Department> department;
    QString initialName;
};

#endif // DEPARTMENTEDITDIALOG_H
