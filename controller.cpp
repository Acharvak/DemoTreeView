#include "controller.h"

#include <QSet>
#include <stdexcept>

namespace TreeDemo {

Controller::ChangeEvent::ChangeEvent(bool is_starting, bool is_final, Type type,
                                     ModelItemPtr arg0, ModelItemPtr arg1,
                                     ModelItemPtr arg2)
    : isStarting(is_starting), isFinal(is_final), type(type),
      args({arg0, arg1, arg2}) {}

Controller::Controller(Controller&& other) {
    this->operator=(std::move(other));
}

Controller& Controller::operator=(Controller&& other) {
    if(this != &other) {
        undoQueue = std::move(other.undoQueue);
        redoQueue = std::move(other.redoQueue);
    }
    return *this;
}

Controller::~Controller() { reset(); }

bool Controller::canUndo() const { return !undoQueue.empty(); }

bool Controller::canRedo() const { return !redoQueue.empty(); }

void Controller::reset() {
    for(auto evptr: undoQueue) {
        delete evptr;
    }
    undoQueue.clear();
    clearRedoQueue();
}

void Controller::replaceEmployee(QSharedPointer<Department> department,
                                 QSharedPointer<const Employee> old_employee,
                                 QSharedPointer<const Employee> new_employee) {
    if(!department) {
        throw std::runtime_error("department is null");
    }
    if(old_employee) {
        if(new_employee) {
            // Замена
            department->replaceEmployee(old_employee, new_employee);
        } else {
            // Удаление
            department->unassignEmployee(old_employee);
        }
    } else {
        // Добавление нового
        department->addEmployee(new_employee);
    }
    // Если не было исключений, операция удалась
    prepareToAddUndo();
    undoQueue.append(new ChangeEvent(true, true,
                                     ChangeEvent::Type::REPLACE_EMPLOYEE,
                                     department, old_employee, new_employee));
}

void Controller::clearRedoQueue() {
    for(auto evptr: redoQueue) {
        delete evptr;
    }
    redoQueue.clear();
}

void Controller::prepareToAddUndo() {
    clearRedoQueue();
    // Чтобы можно было сделать бесконечную очередь
    if(!MAX_QUEUE_LENGTH) {
        return;
    }
    while(undoQueue.size() > MAX_QUEUE_LENGTH) {
        bool sequence_ended{false};
        while(!sequence_ended) {
            auto evptr = undoQueue.takeFirst();
            sequence_ended = evptr->isFinal;
            delete evptr;
        };
    }
}

void Controller::massDeletion(QList<ModelItemPtr> items) {
    if(items.isEmpty()) {
        return;
    }
    QSet<Department*> depset{};
    for(auto miptr: items) {
        if(miptr.get()->getType() == ModelItem::Type::DEPARTMENT) {
            depset.insert(miptr.asDepartment().get());
        }
    }
    QList<QSharedPointer<const Employee>> e2rm{};
    QList<QSharedPointer<Department>> d2rm{};
    for(auto miptr: items) {
        auto rawptr = miptr.get();
        if(!depset.contains(rawptr->getDepartment().toStrongRef().get())) {
            if(rawptr->getType() == ModelItem::Type::DEPARTMENT) {
                d2rm.append(miptr.asDepartment());
            } else {
                e2rm.append(miptr.asEmployee());
            }
        }
    }
    prepareToAddUndo();
    bool is_starting{true};
    for(auto eptr: e2rm) {
        auto parent = eptr->getDepartment().toStrongRef();
        parent->unassignEmployee(eptr);
        undoQueue.append(new ChangeEvent(is_starting, false,
                                         ChangeEvent::Type::REPLACE_EMPLOYEE,
                                         parent, eptr));
        is_starting = false;
    }
    for(auto dptr: d2rm) {
        auto parent = dptr->getDepartment().toStrongRef();
        parent->unassignSubdepartment(dptr);
        undoQueue.append(new ChangeEvent(is_starting, false,
                                         ChangeEvent::Type::MOVE_DEPARTMENT,
                                         dptr, parent));
        is_starting = false;
    }
    if(!is_starting) {
        // Если добавлено хоть одно событие
        undoQueue.back()->isFinal = true;
    }
}

void Controller::addDepartment(QSharedPointer<Department> parent,
                               QString name) {
    auto result = Department::create(name, parent);
    prepareToAddUndo();
    undoQueue.append(new ChangeEvent(true, true,
                                     ChangeEvent::Type::MOVE_DEPARTMENT, result,
                                     ModelItemPtr(), parent));
}

void Controller::renameDepartment(QSharedPointer<Department> department,
                                  QString new_name) {
    if(department->getName() == new_name) {
        return;
    }
    auto old_name = department->rename(new_name);
    prepareToAddUndo();
    auto onstore = Department::create(old_name);
    auto nnstore = Department::create(new_name);
    undoQueue.append(new ChangeEvent(true, true,
                                     ChangeEvent::Type::RENAME_DEPARTMENT,
                                     department, onstore, nnstore));
}

void Controller::undo() {
    if(!canUndo()) {
        return;
    }
    bool sequence_ended{false};
    while(!sequence_ended) {
        auto evptr = undoQueue.takeLast();
        auto department = evptr->args.at(0).asDepartment();
        switch(evptr->type) {
        case ChangeEvent::Type::MOVE_DEPARTMENT:
            if(evptr->args.at(1)) {
                auto old_parent = evptr->args.at(1).asDepartment();
                if(evptr->args.at(2)) {
                    // Это перемещение
                    auto new_parent = evptr->args.at(2).asDepartment();
                    new_parent->unassignSubdepartment(department);
                } // Иначе это удаление
                old_parent->addSubdepartment(department);
            } else {
                // Это добавление
                auto new_parent = evptr->args.at(2).asDepartment();
                new_parent->unassignSubdepartment(
                    evptr->args.at(0).asDepartment());
            }
            break;
        case ChangeEvent::Type::REPLACE_EMPLOYEE:
            if(evptr->args.at(1)) {
                auto old_employee = evptr->args.at(1).asEmployee();
                if(evptr->args.at(2)) {
                    // Это замена
                    department->replaceEmployee(evptr->args.at(2).asEmployee(),
                                                old_employee);
                } else {
                    // Это удаление
                    department->addEmployee(old_employee);
                }
            } else {
                // Это добавление нового сотрудника
                department->unassignEmployee(evptr->args.at(2).asEmployee());
            }
            break;
        case ChangeEvent::Type::RENAME_DEPARTMENT:
            department->rename(evptr->args.at(1).asDepartment()->getName());
            break;
        }
        sequence_ended = evptr->isFinal;
        redoQueue.append(evptr);
    }
}

void Controller::redo() {
    if(!canRedo()) {
        return;
    }
    bool sequence_ended{false};
    while(!sequence_ended) {
        auto evptr = redoQueue.takeLast();
        auto department = evptr->args.at(0).asDepartment();
        switch(evptr->type) {
        case ChangeEvent::Type::MOVE_DEPARTMENT:
            if(evptr->args.at(1)) {
                auto current_parent = evptr->args.at(1).asDepartment();
                current_parent->unassignSubdepartment(department);
                if(evptr->args.at(2)) {
                    // Это перемещение
                    auto new_parent = evptr->args.at(2).asDepartment();
                    new_parent->addSubdepartment(department);
                } // Иначе это удаление
            } else {
                // Это добавление
                auto new_parent = evptr->args.at(2).asDepartment();
                new_parent->addSubdepartment(evptr->args.at(0).asDepartment());
            }
            break;
        case ChangeEvent::Type::REPLACE_EMPLOYEE:
            if(evptr->args.at(1)) {
                auto current_employee = evptr->args.at(1).asEmployee();
                if(evptr->args.at(2)) {
                    // Это замена
                    department->replaceEmployee(current_employee,
                                                evptr->args.at(2).asEmployee());
                } else {
                    // Это удаление
                    department->unassignEmployee(current_employee);
                }
            } else {
                // Это добавление нового сотрудника
                department->addEmployee(evptr->args.at(2).asEmployee());
            }
            break;
        case ChangeEvent::Type::RENAME_DEPARTMENT:
            department->rename(evptr->args.at(2).asDepartment()->getName());
            break;
        }
        sequence_ended = evptr->isStarting;
        undoQueue.append(evptr);
    }
}

} // namespace TreeDemo
