#include "employeeeditdialog.h"
#include "ui_employeeeditdialog.h"

#include <QMessageBox>
#include <stdexcept>

EmployeeEditDialog::EmployeeEditDialog(QWidget* parent, Controller& controller,
                                       QSharedPointer<Department> department,
                                       QSharedPointer<const Employee> employee)
    : QDialog(parent), ui(new Ui::EmployeeEditDialog), controller(controller),
      department(department), employee(employee) {
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    ui->departmentEditRO->setText(department->getName());
    if(employee) {
        setWindowTitle(tr("Editing employee info"));
        initialSurname = employee->surname;
        ui->surnameEdit->setText(initialSurname);
        initialName = employee->name;
        ui->nameEdit->setText(initialName);
        initialMiddleName = employee->middleName;
        ui->middleNameEdit->setText(initialMiddleName);
        initialFunction = employee->function;
        ui->functionEdit->setText(initialFunction);
        initialSalary = QString("%1").arg(employee->salary / 100);
        ui->salaryEdit->setText(initialSalary);
    } else {
        setWindowTitle(tr("New employee"));
        initialSurname = "";
        initialName = "";
        initialMiddleName = "";
        initialSalary = "";
    }

    QObject::connect(ui->saveButton, &QPushButton::clicked, this,
                     &EmployeeEditDialog::saveButtonClicked);
    QObject::connect(ui->cancelButton, &QPushButton::clicked, this,
                     &EmployeeEditDialog::cancelButtonClicked);

    QLineEdit* edits[]{ui->surnameEdit, ui->nameEdit, ui->middleNameEdit,
                       ui->functionEdit, ui->salaryEdit};
    for(auto editptr: edits) {
        QObject::connect(editptr, &QLineEdit::textChanged, this,
                         &EmployeeEditDialog::textChanged);
    }
}

EmployeeEditDialog::~EmployeeEditDialog() { delete ui; }

void EmployeeEditDialog::saveButtonClicked([[maybe_unused]] bool checked) {
    try {
        bool success{true};
        auto salary = ui->salaryEdit->text().toULongLong(&success);
        if(!success) {
            throw std::invalid_argument(
                tr("Invalid salary value").toStdString());
        }
        auto new_employee = Employee::create(
            ui->surnameEdit->text(), ui->nameEdit->text(),
            ui->middleNameEdit->text(), ui->functionEdit->text(), salary * 100);
        controller.replaceEmployee(department, employee, new_employee);
        accept();
    } catch(std::invalid_argument& e) {
        QMessageBox msg(QMessageBox::Icon::Critical, tr("Error"), e.what(),
                        QMessageBox::Ok, this);
        msg.exec();
    }
}

void EmployeeEditDialog::cancelButtonClicked([[maybe_unused]] bool checked) {
    reject();
}

void EmployeeEditDialog::textChanged([[maybe_unused]] const QString& text) {
    ui->saveButton->setEnabled(ui->surnameEdit->text() != initialSurname ||
                               ui->nameEdit->text() != initialName ||
                               ui->middleNameEdit->text() !=
                                   initialMiddleName ||
                               ui->functionEdit->text() != initialFunction ||
                               ui->salaryEdit->text() != initialSalary);
}
