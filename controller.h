#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "model.h"

#include <QList>
#include <array>

namespace TreeDemo {

class Controller {
  public:
    static const int MAX_QUEUE_LENGTH{100000};

    Controller() = default;
    Controller(Controller&) = delete;
    Controller(Controller&& other);
    ~Controller();

    Controller& operator=(Controller&) = delete;
    Controller& operator=(Controller&& other);

    bool canUndo() const;
    bool canRedo() const;

    void reset();

    /**
     * @brief Добавить, заменить или удалить сотрудника
     * @param department подразделение; не должно быть nullptr
     * @param old_employee заменяемый или удаляемый сотрудник, nullptr если
     * добавляется новый
     * @param new_employee добавляемый сотрудник, nullptr если старый просто
     * удаляется
     * @throw std::invalid_argument из вызываемых методов department
     */
    void replaceEmployee(QSharedPointer<Department> department,
                         QSharedPointer<const Employee> old_employee,
                         QSharedPointer<const Employee> new_employee);

    /**
     * @brief Удалить вхождения из дерева
     *
     * Можно передать список Employee и Department. Если в списке есть и
     * сотрудник/субподразделение, и подразделение, куда они входят, метод
     * корректно обработает ситуацию и удалит только самое «внешнее»
     * пподразделение.
     */
    void massDeletion(QList<ModelItemPtr> items);

    void addDepartment(QSharedPointer<Department> parent, QString name);
    void renameDepartment(QSharedPointer<Department> department,
                          QString new_name);

    void undo();
    void redo();

  private:
    class ChangeEvent {
      public:
        enum class Type {
            // Арг. 0 = подразделение, 1 = заменённый/удалённый (если есть), 2 =
            // новый (если есть)
            REPLACE_EMPLOYEE,
            // Арг. 0 = подразделение, 1 = фиктивное подразделение для хранения
            // старого названия, 2 = для нового
            RENAME_DEPARTMENT,
            // Арг. 0 = субподразделение, 1 = старый родитель (если есть), 2 =
            // новый (если есть)
            MOVE_DEPARTMENT,
        };
        bool isStarting;
        bool isFinal;
        Type type;
        std::array<ModelItemPtr, 3> args;

        ChangeEvent(bool is_starting, bool is_final, Type type,
                    ModelItemPtr arg0 = ModelItemPtr(),
                    ModelItemPtr arg1 = ModelItemPtr(),
                    ModelItemPtr arg2 = ModelItemPtr());
        ChangeEvent(ChangeEvent&) = delete;
        ChangeEvent(ChangeEvent&&) = delete;
    };

    // Подразумеваются уникальные указатели. Поскольку Qt не гарантирует
    // взаимодействуя своих контейнеров и move-семантики, пользуемся обычными.
    QList<ChangeEvent*> undoQueue{};
    QList<ChangeEvent*> redoQueue{};

    void clearRedoQueue();
    void prepareToAddUndo();
};

} // namespace TreeDemo

#endif // CONTROLLER_H
