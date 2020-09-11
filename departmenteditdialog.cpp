#include "departmenteditdialog.h"
#include "ui_departmenteditdialog.h"

#include <QMessageBox>
#include <stdexcept>

DepartmentEditDialog::DepartmentEditDialog(
    QWidget* parent, Controller& controller,
    QSharedPointer<Department> parent_department,
    QSharedPointer<Department> department)
    : QDialog(parent), ui(new Ui::DepartmentEditDialog), controller(controller),
      parentDepartment(parent_department), department(department) {
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    if(parent_department->getDepartment()) {
        ui->parentNameEditRO->setText(parent_department->getName());
    } else {
        ui->parentNameEditRO->setText(tr("(organization)"));
    }
    if(department) {
        initialName = department->getName();
        setWindowTitle(tr("Renaming department"));
        ui->nameEdit->setText(department->getName());
    } else {
        initialName = "";
        setWindowTitle(tr("Adding department"));
    }
    QObject::connect(ui->nameEdit, &QLineEdit::textChanged, this,
                     &DepartmentEditDialog::nameChanged);
    QObject::connect(ui->cancelButton, &QPushButton::clicked, this,
                     &DepartmentEditDialog::cancelButtonClicked);
    QObject::connect(ui->saveButton, &QPushButton::clicked, this,
                     &DepartmentEditDialog::saveButtonClicked);
}

DepartmentEditDialog::~DepartmentEditDialog() { delete ui; }

void DepartmentEditDialog::nameChanged(const QString& text) {
    ui->saveButton->setEnabled(text != initialName);
}

void DepartmentEditDialog::cancelButtonClicked([[maybe_unused]] bool checked) {
    reject();
}

void DepartmentEditDialog::saveButtonClicked([[maybe_unused]] bool checked) {
    auto name = ui->nameEdit->text();
    if(name == initialName) {
        return;
    }
    try {
        if(department) {
            controller.renameDepartment(department, name);
        } else {
            controller.addDepartment(parentDepartment, name);
        }
        accept();
    } catch(std::invalid_argument& e) {
        QMessageBox msg(QMessageBox::Icon::Critical, tr("Error"), e.what(),
                        QMessageBox::Ok, this);
        msg.exec();
    }
}
