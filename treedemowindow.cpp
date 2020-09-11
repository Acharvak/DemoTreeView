#include "treedemowindow.h"
#include "./ui_treedemowindow.h"
#include "departmenteditdialog.h"
#include "employeeeditdialog.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QSaveFile>
#include <QTranslator>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <cstdlib>
#include <stdexcept>

QString TreeDemoWindow::getFixed100(qulonglong value) {
    // Мы предполагаем, что формат валюты входит в перевод
    return tr("%1.%2")
        .arg(value / 100)
        .arg(value % 100, 2, 10, QLatin1Char('0'));
}

TreeDemoWindow::TreeDemoWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::TreeDemoWindow) {
    ui->setupUi(this);
    QObject::connect(ui->companyTree, &QTreeWidget::itemSelectionChanged, this,
                     &TreeDemoWindow::VE_itemSelectionChanged);
    QObject::connect(ui->loadButton, &QPushButton::clicked, this,
                     &TreeDemoWindow::VE_loadButtonClicked);
    QObject::connect(ui->saveButton, &QPushButton::clicked, this,
                     &TreeDemoWindow::VE_saveButtonClicked);
    QObject::connect(ui->subdepartmentsCheckBox, &QCheckBox::clicked, this,
                     &TreeDemoWindow::VE_subdepartmentsCheckboxClicked);
    QObject::connect(ui->editButton, &QPushButton::clicked, this,
                     &TreeDemoWindow::VE_editButtonClicked);
    QObject::connect(ui->deleteButton, &QPushButton::clicked, this,
                     &TreeDemoWindow::VE_deleteButtonClicked);
    QObject::connect(ui->newEmployeeButton, &QPushButton::clicked, this,
                     &TreeDemoWindow::VE_newEmployeeButtonClicked);
    QObject::connect(ui->newDepartmentButton, &QPushButton::clicked, this,
                     &TreeDemoWindow::VE_newDepartmentButtonClicked);
    QObject::connect(ui->undoButton, &QPushButton::clicked, this,
                     &TreeDemoWindow::VE_undoButtonClicked);
    QObject::connect(ui->redoButton, &QPushButton::clicked, this,
                     &TreeDemoWindow::VE_redoButtonClicked);
    setModel(Department::create("<ROOT>"));
}

TreeDemoWindow::~TreeDemoWindow() { delete ui; }

void TreeDemoWindow::loadXMLFile(QString path) {
    try {
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly)) {
            throw std::runtime_error(file.errorString().toStdString());
        }
        auto data = file.readAll();
        if(data.isEmpty()) {
            throw std::runtime_error(file.errorString().toStdString());
        }
        QDomDocument document{};
        QString errstr{};
        document.setContent(data, false, &errstr);
        if(!errstr.isEmpty()) {
            throw std::runtime_error(errstr.toStdString());
        }
        auto xmlroot = document.documentElement();
        if(xmlroot.tagName() != "departments") {
            throw std::runtime_error(
                tr("Bad XML file: the root element is not <departments>")
                    .toStdString());
        }
        auto root = Department::create("<ROOT>");
        auto children = xmlroot.childNodes();
        for(int i{0}; i < children.size(); ++i) {
            auto cn = children.at(i);
            if(cn.isElement()) {
                auto cne = cn.toElement();
                if(cne.tagName() == "department") {
                    auto subdep = deserialize(cne, root);
                }
            }
        }
        if(root->isModelInconsistent()) {
            throw std::invalid_argument(tr("Surely, you jest?").toStdString());
        }
        setModel(root);
    } catch(std::exception& e) {
        QMessageBox msg(QMessageBox::Icon::Critical, tr("Error"),
                        QString(e.what()), QMessageBox::Ok, this);
        msg.exec();
    }
}

void TreeDemoWindow::saveXMLFile(QString path) {
    try {
        QSaveFile file(path);
        if(!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error(file.errorString().toStdString());
        }
        QDomDocument xml{};
        serializeDepartment(xml, xml, nullptr);
        auto data = xml.toByteArray();
        file.write(data);
        if(!file.commit()) {
            throw std::runtime_error(file.errorString().toStdString());
        }
    } catch(std::exception& e) {
        QMessageBox msg(QMessageBox::Icon::Critical, tr("Error"),
                        QString(e.what()), QMessageBox::Ok, this);
        msg.exec();
    }
}

QSharedPointer<const Employee>
TreeDemoWindow::deserialize(QDomElement employment) {
    QString surname{employment.firstChildElement("surname").text()};
    QString name{employment.firstChildElement("name").text()};
    QString middle_name{employment.firstChildElement("middleName").text()};
    QString function{employment.firstChildElement("function").text()};
    QString salary_str{employment.firstChildElement("salary").text()};
    bool salary_ok{true};
    qulonglong salary{
        salary_str.isEmpty() ? 0 : salary_str.toULongLong(&salary_ok)};
    if(!salary_ok || salary > MAX_EMPLOYEE_SALARY / 100) {
        throw std::invalid_argument(
            tr("Invalid salary: %1").arg(salary_str).toStdString());
    }
    // В XML salary в рублях, а в модели в копейках
    return Employee::create(surname, name, middle_name, function, salary * 100);
}

QSharedPointer<Department>
TreeDemoWindow::deserialize(QDomElement department,
                            QSharedPointer<Department> parent) {
    auto result = Department::create(department.attribute("name"));
    auto children = department.firstChildElement("departments").childNodes();
    // QDomNodeList не поддерживает итерацию
    for(int i{0}; i < children.size(); ++i) {
        auto cn = children.at(i);
        if(cn.isElement()) {
            auto cne = cn.toElement();
            if(cne.tagName() == "department") {
                auto subdep = deserialize(cne, result);
                // Подразделение добавится автоматически
            }
        }
    }
    children = department.firstChildElement("employments").childNodes();
    for(int i{0}; i < children.size(); ++i) {
        auto cn = children.at(i);
        if(cn.isElement()) {
            auto cne = cn.toElement();
            if(cne.tagName() == "employment") {
                auto employee = deserialize(cne);
                result->addEmployee(employee);
            }
        }
    }
    // Добавляем в родителя здесь, чтобы если раньше выпадет исключение,
    // то всё бы уничтожилось
    if(parent) {
        parent->addSubdepartment(result);
    }
    return result;
}

TreeDemoWindow::SelectionType
TreeDemoWindow::getSelectionType(const QList<QTreeWidgetItem*> selection) {
    SelectionType seltype;
    if(selection.size() == 0) {
        seltype = SelectionType::NONE;
    } else if(selection.size() == 1) {
        auto item = translateTree2Model(selection.first()).get();
        if(item->getType() == ModelItem::Type::DEPARTMENT) {
            seltype = SelectionType::DEPARTMENT;
        } else {
            seltype = SelectionType::EMPLOYEE;
        }
    } else {
        // Либо SEVERAL_EMPLOYEES, либо INCOHERENT
        seltype = SelectionType::SEVERAL_EMPLOYEES;
        for(auto selitem: selection) {
            auto item = translateTree2Model(selitem).get();
            if(item->getType() != ModelItem::Type::EMPLOYEE) {
                seltype = SelectionType::INCOHERENT;
                break;
            }
        }
    }
    return seltype;
}

void TreeDemoWindow::resetUndoRedo() {
    ui->undoButton->setEnabled(controller.canUndo());
    ui->redoButton->setEnabled(controller.canRedo());
}

void TreeDemoWindow::setModel(QSharedPointer<Department> root) {
    if(modelRoot) {
        for(size_t i{0}; i < modelRoot->getNumSubdepartments(); ++i) {
            removeDepartmentView(modelRoot->getSubdepartment(i).get(), nullptr,
                                 false);
        }
        QObject::disconnect(modelRoot.get(), nullptr, this, nullptr);
    }

    modelRoot = root;
    QObject::connect(root.get(), &Department::employeesChanged, this,
                     &TreeDemoWindow::ME_employeesChanged);
    QObject::connect(root.get(), &Department::departmentsChanged, this,
                     &TreeDemoWindow::ME_departmentsChanged);

    ui->companyTree->clear();
    tree2model.clear();
    model2tree.clear();
    cutItems.clear();
    controller.reset();

    for(size_t i{0}; i < root->getNumSubdepartments(); ++i) {
        addDepartmentView(root->getSubdepartment(i),
                          ui->companyTree->invisibleRootItem());
    }

    VE_itemSelectionChanged();
    resetUndoRedo();
}

ModelItemPtr TreeDemoWindow::translateTree2Model(QTreeWidgetItem* twitem) {
    auto result = tree2model.value(twitem);
    if(!result) {
        QMessageBox msg(QMessageBox::Icon::Critical, tr("Internal error"),
                        tr("INTERNAL ERROR: View and model desynchronized"),
                        QMessageBox::Ok, this);
        msg.exec();
        abort();
    }
    return result;
}

QTreeWidgetItem* TreeDemoWindow::translateModel2Tree(const ModelItem* mitem) {
    auto result = model2tree.value(mitem);
    if(!result) {
        QMessageBox msg(QMessageBox::Icon::Critical, tr("Internal error"),
                        tr("INTERNAL ERROR: View and model desynchronized"),
                        QMessageBox::Ok, this);
        msg.exec();
        abort();
    }
    return result;
}

void TreeDemoWindow::addDepartmentView(QSharedPointer<Department> department,
                                       QTreeWidgetItem* parent_view) {
    if(!parent_view) {
        auto parent = department->getDepartment();
        if(parent == modelRoot) {
            parent_view = ui->companyTree->invisibleRootItem();
        } else {
            parent_view = translateModel2Tree(parent.toStrongRef().get());
        }
    }
    // По-хорошему, нужно делать иконки, но это уже слишком для
    // такого тестового задания
    QStringList columns{"", "", ""};
    auto view = new QTreeWidgetItem(parent_view, columns);
    tree2model.insert(view, department);
    model2tree.insert(department.get(), view);
    QObject::connect(department.get(), &Department::employeesChanged, this,
                     &TreeDemoWindow::ME_employeesChanged);
    QObject::connect(department.get(), &Department::departmentsChanged, this,
                     &TreeDemoWindow::ME_departmentsChanged);
    QObject::connect(department.get(), &Department::modelRecalculated, this,
                     &TreeDemoWindow::ME_modelRecalculated);
    QObject::connect(department.get(), &Department::inconsistentModel, this,
                     &TreeDemoWindow::ME_inconsistentModel);
    updateDepartmentView(department.get(), view);
    for(size_t i{0}; i < department->getNumSubdepartments(); ++i) {
        auto subdep = department->getSubdepartment(i);
        addDepartmentView(subdep, view);
    }
    for(size_t i{0}; i < department->getNumEmployees(); ++i) {
        auto employee = department->getEmployee(i);
        addEmployeeView(employee, view);
    }
}

void TreeDemoWindow::updateDepartmentView(const Department* department,
                                          QTreeWidgetItem* view) {
    if(!view) {
        view = translateModel2Tree(department);
    }
    size_t total_employees;
    qulonglong mean_salary;
    if(ui->subdepartmentsCheckBox->isChecked()) {
        total_employees = department->getNumEmployeesWithSubdepartments();
        mean_salary = department->getMeanSalaryWithSubdepartments();
    } else {
        total_employees = department->getNumEmployees();
        mean_salary = department->getMeanSalary();
    }
    // Символ = 📜
    view->setText(0, QString("\360\237\223\234") + department->getName());
    if(total_employees == 1) {
        view->setText(1, tr("1 employee"));
    } else {
        view->setText(1, tr("%n employees", "", total_employees));
    }
    view->setText(2, getFixed100(mean_salary));
}

void TreeDemoWindow::removeDepartmentView(Department* department,
                                          QTreeWidgetItem* view,
                                          bool delete_view) {
    if(!view) {
        view = translateModel2Tree(department);
    }
    QObject::disconnect(department, nullptr, this, nullptr);
    for(size_t i{0}; i < department->getNumSubdepartments(); ++i) {
        removeDepartmentView(department->getSubdepartment(i).get(), nullptr,
                             false);
    }
    model2tree.remove(department);
    tree2model.remove(view);
    if(delete_view) {
        delete view;
    }
}

void TreeDemoWindow::addEmployeeView(QSharedPointer<const Employee> employee,
                                     QTreeWidgetItem* parent_view) {
    // В соответствии с XML сотрудников в корне быть не может
    if(!parent_view) {
        parent_view =
            translateModel2Tree(employee->getDepartment().toStrongRef().get());
    }
    QStringList columns{"", "", ""};
    auto view = new QTreeWidgetItem(parent_view, columns);
    tree2model.insert(view, employee);
    model2tree.insert(employee.get(), view);
    updateEmployeeView(employee.get(), view);
}

void TreeDemoWindow::updateEmployeeView(const Employee* employee,
                                        QTreeWidgetItem* view) {
    if(!view) {
        view = translateModel2Tree(employee);
    }
    // Символ = 👤
    view->setText(0, QString("\360\237\221\244") + employee->fullName);
    view->setText(1, employee->function);
    view->setText(2, getFixed100(employee->salary));
}

void TreeDemoWindow::removeEmployeeView(const Employee* employee,
                                        QTreeWidgetItem* view,
                                        bool actually_remove) {
    // Удаление сотрудника
    model2tree.remove(employee);
    tree2model.remove(view);
    if(actually_remove) {
        delete view;
    }
}

void TreeDemoWindow::VE_itemSelectionChanged() {
    // Исследуем тип выборки
    auto selection = ui->companyTree->selectedItems();
    auto seltype = getSelectionType(selection);

    // Переключаем кнопки
    ui->editButton->setEnabled(selection.size() == 1);
    /*
    ui->cutButton->setEnabled(seltype != SelectionType::NONE
            && seltype != SelectionType::INCOHERENT);
    ui->pasteButton->setEnabled(seltype == SelectionType::DEPARTMENT
                                && !cutItems.isEmpty());
    */
    ui->deleteButton->setEnabled(seltype != SelectionType::NONE);
    // На высоком уровне должны быть только подразделения, судя по XML
    ui->newEmployeeButton->setEnabled(seltype == SelectionType::DEPARTMENT);
    ui->newDepartmentButton->setEnabled(seltype == SelectionType::NONE ||
                                        seltype == SelectionType::DEPARTMENT);
}

void TreeDemoWindow::VE_loadButtonClicked([[maybe_unused]] bool checked) {
    auto fname = QFileDialog::getOpenFileName(
        this, tr("Choose file to load"), "", tr("XML files (*.xml)"), nullptr,
        QFileDialog::Option::ReadOnly);
    if(!fname.isEmpty()) {
        if(savePathHint.isEmpty()) {
            savePathHint = fname;
        }
        loadXMLFile(fname);
    }
}

void TreeDemoWindow::VE_saveButtonClicked([[maybe_unused]] bool checked) {
    if(savePathHint.isEmpty()) {
        savePathHint = "company.xml";
    }
    auto fname =
        QFileDialog::getSaveFileName(this, tr("Choose where to save"),
                                     savePathHint, tr("XML files (*.xml)"));
    if(!fname.isEmpty()) {
        saveXMLFile(fname);
    }
}

void TreeDemoWindow::VE_subdepartmentsCheckboxClicked([
    [maybe_unused]] bool checked) {
    for(size_t i{0}; i < modelRoot->getNumSubdepartments(); ++i) {
        updateDepartmentView(modelRoot->getSubdepartment(i).get());
    }
}

void TreeDemoWindow::VE_editButtonClicked([[maybe_unused]] bool checked) {
    auto selection = ui->companyTree->selectedItems();
    if(selection.size() != 1) {
        // Этого не должно происходить, сигнал должен выключить кнопку.
        return;
    }
    auto miptr = translateTree2Model(selection.first());
    if(miptr.get()->getType() == ModelItem::Type::EMPLOYEE) {
        auto employee = miptr.asEmployee();
        EmployeeEditDialog dialog(this, controller, employee->getDepartment(),
                                  employee);
        dialog.exec();
    } else {
        // Подразделение
        auto department = miptr.asDepartment();
        DepartmentEditDialog dialog(this, controller,
                                    department->getDepartment().toStrongRef(),
                                    department);
        dialog.exec();
    }
    resetUndoRedo();
}

void TreeDemoWindow::VE_deleteButtonClicked([[maybe_unused]] bool checked) {
    auto selection = ui->companyTree->selectedItems();
    QList<ModelItemPtr> items{};
    for(auto twitemptr: selection) {
        items.append(translateTree2Model(twitemptr));
    }
    controller.massDeletion(items);
    resetUndoRedo();
}

void TreeDemoWindow::VE_newEmployeeButtonClicked([
    [maybe_unused]] bool checked) {
    auto selection = ui->companyTree->selectedItems();
    if(selection.size() != 1) {
        return;
    }
    auto miptr = translateTree2Model(selection.first());
    if(miptr.get()->getType() != ModelItem::Type::DEPARTMENT) {
        // Не должно происходить
        return;
    }
    auto department = miptr.asDepartment();
    EmployeeEditDialog dialog(this, controller, department, nullptr);
    dialog.exec();
    resetUndoRedo();
}

void TreeDemoWindow::VE_newDepartmentButtonClicked([
    [maybe_unused]] bool checked) {
    auto selection = ui->companyTree->selectedItems();
    if(selection.size() == 0) {
        DepartmentEditDialog dialog(this, controller, modelRoot, nullptr);
        dialog.exec();
    } else if(selection.size() == 1) {
        auto miptr = translateTree2Model(selection.first());
        if(miptr.get()->getType() != ModelItem::Type::DEPARTMENT) {
            return;
        }
        auto department = miptr.asDepartment();
        DepartmentEditDialog dialog(this, controller, department, nullptr);
        dialog.exec();
        resetUndoRedo();
    } else {
        return;
    }
}

void TreeDemoWindow::VE_undoButtonClicked([[maybe_unused]] bool checked) {
    controller.undo();
    resetUndoRedo();
}

void TreeDemoWindow::VE_redoButtonClicked([[maybe_unused]] bool checked) {
    controller.redo();
    resetUndoRedo();
}

void TreeDemoWindow::ME_modelRecalculated(Department* where) {
    updateDepartmentView(where);
}

void TreeDemoWindow::ME_employeesChanged(
    [[maybe_unused]] Department* where,
    QSharedPointer<const Employee> removed_employee,
    QSharedPointer<const Employee> new_employee) {
    if(removed_employee) {
        auto view = translateModel2Tree(removed_employee.get());
        if(new_employee) {
            // Замена сотрудника
            model2tree.remove(removed_employee.get());
            model2tree.insert(new_employee.get(), view);
            tree2model.remove(view);
            tree2model.insert(view, new_employee);
            updateEmployeeView(new_employee.get(), view);
        } else {
            removeEmployeeView(removed_employee.get(), view);
        }
    } else if(new_employee) {
        // Добавление сотрудника
        addEmployeeView(new_employee);
    }
    // Третьего в общем-то не должно быть дано, но на всякий случай проверяем
}

void TreeDemoWindow::ME_departmentsChanged(
    Department* where, Department::Change what,
    [[maybe_unused]] QString old_name,
    QSharedPointer<Department> subdepartment) {
    switch(what) {
    case Department::Change::NAME_CHANGED:
        updateDepartmentView(where);
        break;
    case Department::Change::SUBDEPARTMENT_ADDED:
        addDepartmentView(subdepartment);
        break;
    case Department::Change::SUBDEPARTMENT_REMOVED:
        removeDepartmentView(subdepartment.get());
        break;
    default:
        // Недостижимо
        abort();
    }
}

void TreeDemoWindow::ME_inconsistentModel() {
    QMessageBox msg(QMessageBox::Icon::Critical, tr("Error"),
                    tr("Surely, you jest?"), QMessageBox::Yes, this);
    msg.exec();
    abort();
}

void TreeDemoWindow::serializeDepartment(QDomDocument& document,
                                         QDomNode& target,
                                         QTreeWidgetItem* twitem) {
    QSharedPointer<Department> department;
    if(twitem) {
        department = translateTree2Model(twitem).asDepartment();
    } else {
        twitem = ui->companyTree->invisibleRootItem();
        department = modelRoot;
    }
    auto departments_xml = QDomElement();
    auto employments_xml = QDomElement();
    for(int i{0}; i < twitem->childCount(); ++i) {
        auto twchild = twitem->child(i);
        auto child = translateTree2Model(twchild);
        if(child.get()->getType() == ModelItem::Type::EMPLOYEE) {
            if(employments_xml.isNull()) {
                employments_xml =
                    std::move(document.createElement("employments"));
                target.appendChild(employments_xml);
            }
            auto employee = child.asEmployee();
            auto employee_xml = document.createElement("employment");
            employments_xml.appendChild(employee_xml);
            // clang-format off
            const QStringList elements {
                "surname", employee->surname,
                "name", employee->name,
                "middleName", employee->middleName,
                "function", employee->function
            };
            // clang-format on
            for(int i{0}; i < elements.size(); i += 2) {
                auto node = document.createElement(elements.at(i));
                employee_xml.appendChild(node);
                auto text = document.createTextNode(elements.at(i + 1));
                node.appendChild(text);
            }
            auto salary_xml = document.createElement("salary");
            employee_xml.appendChild(salary_xml);
            auto salary = document.createTextNode(
                QString("%1").arg(employee->salary / 100));
            salary_xml.appendChild(salary);
        } else {
            if(departments_xml.isNull()) {
                departments_xml =
                    std::move(document.createElement("departments"));
                target.appendChild(departments_xml);
            }
            auto department_xml = document.createElement("department");
            departments_xml.appendChild(department_xml);
            department_xml.setAttribute("name",
                                        child.asDepartment()->getName());
            serializeDepartment(document, department_xml, twchild);
        }
    }
}
