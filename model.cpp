#include "model.h"

#include <QLocale>
#include <QStringBuilder>
#include <QTranslator>
#include <limits>
#include <stdexcept>

namespace TreeDemo {
QWeakPointer<Department> ModelItem::getDepartment() const { return department; }

size_t ModelItem::getIndexInDepartment() const { return indexInDepartment; }

QSharedPointer<const Employee> Employee::create(QString surname, QString name,
                                                QString middle_name,
                                                QString function,
                                                qulonglong salary) {
    if(salary > MAX_EMPLOYEE_SALARY) {
        throw std::invalid_argument(
            tr("The salary exceeds maximum permitted value").toStdString());
    }
    surname = surname.trimmed();
    name = name.trimmed();
    middle_name = middle_name.trimmed();
    if(surname.isEmpty() && name.isEmpty() && middle_name.isEmpty()) {
        throw std::invalid_argument(tr("An employee must have at least a first "
                                       "name, a last name or a patronymic")
                                        .toStdString());
    }
    return QSharedPointer<const Employee>(
        new Employee(surname, name, middle_name, function, salary));
}

ModelItem::Type Employee::getType() const { return Type::EMPLOYEE; }

Employee::Employee(QString surname, QString name, QString middle_name,
                   QString function, qulonglong salary)
    : surname(surname), name(name), middleName(middle_name),
      fullName(QString("%1 %2 %3").arg(surname, name, middle_name).trimmed()),
      function(function), salary(salary){};

QSharedPointer<Department>
Department::create(QString name, QSharedPointer<Department> parent) {
    name = name.trimmed();
    if(name.isEmpty()) {
        throw std::invalid_argument(
            tr("Department name is empty").toStdString());
    }
    auto new_dept = QSharedPointer<Department>(new Department(name));
    if(parent) {
        parent->addSubdepartment(new_dept);
    }
    return new_dept;
}

Department::Department(QString name) : name(name) {}

ModelItem::Type Department::getType() const { return Type::DEPARTMENT; }

QString Department::getName() const { return name; }

size_t Department::getNumSubdepartments() const {
    return static_cast<size_t>(subdepartments.size());
}

QSharedPointer<Department> Department::getSubdepartment(size_t index) const {
    if(index > std::numeric_limits<int>::max()) {
        // Нарочно не переводится, это ошибка логики
        throw std::out_of_range("Department index out of range");
    }
    auto typed_idx = static_cast<int>(index);
    if(typed_idx >= subdepartments.size()) {
        throw std::out_of_range("Department index out of range");
    }
    return subdepartments.at(typed_idx);
}

QSharedPointer<Department> Department::getSubdepartment(QString name) const {
    name = name.trimmed();
    if(name.isEmpty()) {
        throw std::invalid_argument(
            tr("Department name must not be empty").toStdString());
    }
    return subdepartmentNames.value(name);
}

size_t Department::getNumEmployees() const {
    return static_cast<size_t>(employees.size());
}

QSharedPointer<const Employee> Department::getEmployee(size_t index) const {
    // Придерживаюсь правила копировать код не больше одного раза
    if(index > std::numeric_limits<int>::max()) {
        throw std::out_of_range("Employee index out of range");
    }
    auto typed_idx = static_cast<int>(index);
    if(typed_idx >= employees.size()) {
        throw std::out_of_range("Employee index out of range");
    }
    return employees.at(typed_idx);
}

size_t Department::getNumEmployeesWithSubdepartments() const {
    return totalEmployeesWithSubdeps;
}

qulonglong Department::getTotalSalary() const { return totalSalary; }

qulonglong Department::getTotalSalaryWithSubdepartments() const {
    return totalSalaryWithSubdeps;
}

qulonglong Department::getMeanSalary() const {
    auto ne = getNumEmployees();
    if(ne) {
        return totalSalary / ne;
    } else {
        return 0;
    }
}

qulonglong Department::getMeanSalaryWithSubdepartments() const {
    if(totalEmployeesWithSubdeps) {
        return totalSalaryWithSubdeps / totalEmployeesWithSubdeps;
    } else {
        return 0;
    }
}

QString Department::rename(QString new_name) {
    new_name = new_name.trimmed();
    if(new_name.isEmpty()) {
        throw std::invalid_argument(
            tr("Department name must not be empty").toStdString());
    } else if(new_name == name) {
        return name;
    }

    auto parent_shared = department.toStrongRef();
    if(!parent_shared.isNull()) {
        auto other_dept = parent_shared->getSubdepartment(new_name);
        if(!other_dept.isNull()) {
            throw std::invalid_argument(
                tr("Department \"%1\" already contains a subdepartment named "
                   "\"%2\"")
                    .arg(parent_shared->getName(), new_name)
                    .toStdString());
        }
    }

    auto old_name = name;
    name = new_name;
    emit departmentsChanged(this, Change::NAME_CHANGED, old_name, nullptr);
    return old_name;
}

void Department::addEmployee(QSharedPointer<const Employee> new_employee) {
    auto old_dept = new_employee->department.toStrongRef();
    if(!old_dept.isNull()) {
        throw std::invalid_argument(
            tr("The employee is already assigned to \"%1\"")
                .arg(old_dept->getName())
                .toStdString());
    }
    employees.append(new_employee);
    new_employee->department = sharedFromThis();
    new_employee->indexInDepartment = static_cast<size_t>(employees.size() - 1);
    emit employeesChanged(this, nullptr, new_employee);
    recalculateAfterChanges();
}

void Department::replaceEmployee(QSharedPointer<const Employee> old_employee,
                                 QSharedPointer<const Employee> new_employee) {
    auto old_dept = new_employee->department.toStrongRef();
    if(!old_dept.isNull()) {
        throw std::invalid_argument(
            tr("The employee is already assigned to \"%1\"")
                .arg(old_dept->getName())
                .toStdString());
    }
    old_dept = old_employee->department.toStrongRef();
    if(old_dept != this) {
        throw std::invalid_argument(
            tr("The employee being replaced is not assigned to this department")
                .toStdString());
    }
    auto idx = old_employee->indexInDepartment;
    employees.replace(static_cast<int>(idx), new_employee);
    new_employee->department = sharedFromThis();
    new_employee->indexInDepartment = idx;
    old_employee->department.clear();
    emit employeesChanged(this, old_employee, new_employee);
    recalculateAfterChanges();
}

void Department::unassignEmployee(QSharedPointer<const Employee> employee) {
    auto old_dept = employee->department.toStrongRef();
    if(old_dept != this) {
        throw std::invalid_argument(tr("The employee being unassigned is not "
                                       "assigned to this department")
                                        .toStdString());
    }
    employees.remove(static_cast<int>(employee->indexInDepartment));
    employee->department.clear();
    for(size_t i{0}; i < static_cast<size_t>(employees.size()); ++i) {
        employees.at(static_cast<int>(i))->indexInDepartment = i;
    }
    emit employeesChanged(this, employee, nullptr);
    recalculateAfterChanges();
}

void Department::addSubdepartment(QSharedPointer<Department> new_department) {
    auto old_parent = new_department->getDepartment().toStrongRef();
    if(!old_parent.isNull()) {
        throw std::invalid_argument(
            tr("\"%1\" is already assigned to \"%2\"")
                .arg(new_department->name, old_parent->name)
                .toStdString());
    }
    if(subdepartmentNames.contains(new_department->name)) {
        throw std::invalid_argument(
            tr("\"%1\" already contains a subdepartment named \"%2\"")
                .arg(name, new_department->name)
                .toStdString());
    }
    subdepartments.append(new_department);
    subdepartmentNames.insert(new_department->name, new_department);
    new_department->department = sharedFromThis();
    new_department->indexInDepartment =
        static_cast<size_t>(subdepartments.size() - 1);
    QObject::connect(new_department.get(), &Department::modelRecalculated, this,
                     &Department::slotModelRecalculated);
    QObject::connect(new_department.get(), &Department::departmentsChanged,
                     this, &Department::slotDepartmentsChanged);
    emit departmentsChanged(this, Change::SUBDEPARTMENT_ADDED, name,
                            new_department);
    recalculateAfterChanges();
}

void Department::unassignSubdepartment(QSharedPointer<Department> department) {
    auto old_parent = department->getDepartment().toStrongRef();
    if(old_parent != this) {
        throw std::invalid_argument(tr("\"%1\" is not assigned to \"%2\"")
                                        .arg(department->name, name)
                                        .toStdString());
    }
    subdepartments.remove(department->indexInDepartment);
    subdepartmentNames.remove(department->name);
    department->department.clear();
    QObject::disconnect(department.get(), nullptr, this, nullptr);
    for(size_t i{0}; i < static_cast<size_t>(subdepartments.size()); ++i) {
        subdepartments.at(static_cast<int>(i))->indexInDepartment = i;
    }
    emit departmentsChanged(this, Change::SUBDEPARTMENT_REMOVED, name,
                            department);
    recalculateAfterChanges();
}

void Department::recalculateAfterChanges() {
    auto old_ts = totalSalary;
    auto old_tsws = totalSalaryWithSubdeps;
    auto old_tews = totalEmployeesWithSubdeps;

    totalSalary = 0;
    for(auto& ptr_employee: employees) {
        totalSalary = safeadd(totalSalary, ptr_employee->salary);
    }
    totalSalaryWithSubdeps = totalSalary;
    totalEmployeesWithSubdeps = getNumEmployees();
    for(auto& ptr_subdep: subdepartments) {
        totalEmployeesWithSubdeps =
            safeadd(totalEmployeesWithSubdeps,
                    ptr_subdep->getNumEmployeesWithSubdepartments());
        totalSalaryWithSubdeps =
            safeadd(totalSalaryWithSubdeps,
                    ptr_subdep->getTotalSalaryWithSubdepartments());
    }

    if(old_ts != totalSalary || old_tsws != totalSalaryWithSubdeps ||
       old_tews != totalEmployeesWithSubdeps) {
        emit(modelRecalculated(this));
    }
}

void Department::slotModelRecalculated([[maybe_unused]] Department* where) {
    recalculateAfterChanges();
}

void Department::slotDepartmentsChanged(
    [[maybe_unused]] Department* where, Change what, QString old_name,
    [[maybe_unused]] QSharedPointer<Department> subdepartment) {
    if(what == Change::NAME_CHANGED) {
        auto ptr = subdepartmentNames.take(old_name);
        subdepartmentNames.insert(where->getName(), ptr);
    }
    // Иначе ничего не делать
}

template <class T> T Department::safeadd(T x, T y) {
    auto maxvalue = std::numeric_limits<T>::max();
    if(maxvalue - x < y) {
        if(!inconsistentModelSignaled) {
            emit inconsistentModel();
            inconsistentModelSignaled = true;
        }
        return maxvalue;
    } else {
        return x + y;
    }
}

bool Department::isModelInconsistent() const {
    return inconsistentModelSignaled;
}

ModelItemPtr::ModelItemPtr() : department(nullptr), employee(nullptr) {}

ModelItemPtr::ModelItemPtr(QSharedPointer<Department> department)
    : department(department), employee(nullptr) {}

ModelItemPtr::ModelItemPtr(QSharedPointer<const Employee> employee)
    : department(nullptr), employee(employee) {}

ModelItemPtr::operator bool() const { return department || employee; }

const ModelItem* ModelItemPtr::get() const {
    if(department) {
        return department.get();
    } else {
        return employee.get();
    }
}

QSharedPointer<Department> ModelItemPtr::asDepartment() const {
    if(!department) {
        throw std::runtime_error("ModelItemPtr doesn't point to a Department");
    }
    return department;
}

QSharedPointer<const Employee> ModelItemPtr::asEmployee() const {
    if(!employee) {
        throw std::runtime_error("ModelItemPtr doesn't point to an Employee");
    }
    return employee;
}
} // namespace TreeDemo
