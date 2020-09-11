#ifndef MODEL_H
#define MODEL_H

#include <QAbstractItemModel>
#include <QMap>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QVector>
#include <QtXml/QDomElement>

QT_BEGIN_NAMESPACE
namespace TreeDemo {
class ModelItem;
class Employee;
class Department;
} // namespace TreeDemo
QT_END_NAMESPACE

namespace TreeDemo {
const qulonglong MAX_EMPLOYEE_SALARY{100000000000000000ull};

class ModelItem : public QObject {
  public:
    enum class Type { EMPLOYEE, DEPARTMENT };
    virtual Type getType() const = 0;
    virtual ~ModelItem() {}

    /**
     * @brief Вернуть подразделение, в которое входит данный сотрудник или
     * субподразделение
     *
     * Возвращает nullptr если подразделение ещё не установлено. Устанавливается
     * и меняется оно только методами класса Department. Ссылка эта mutable,
     * поэтому даже const-сотрудника можно перевести в другое подразделение.
     */
    QWeakPointer<Department> getDepartment() const;

    /// Вернуть внутренний индекс в подразделении
    /// Для внутреннего пользования в представлениях
    size_t getIndexInDepartment() const;

  protected:
    mutable QWeakPointer<Department> department;
    mutable size_t indexInDepartment;
};

class Employee : public ModelItem {
    Q_OBJECT

    // Department нужен прямой доступ к полям department и indexInDepartment
    friend Department;

  public:
    /**
     * @brief Создать объект
     * @throw std::invalid_argument если фамилия, имя и отчество все пустые
     *      или если зарплата превышает 100 квадриллионов копеек
     */
    static QSharedPointer<const Employee> create(QString surname, QString name,
                                                 QString middle_name,
                                                 QString function,
                                                 qulonglong salary);

    Employee(Employee&) = delete;
    Employee(Employee&&) = delete;

    Type getType() const override;

    QString surname, name;
    /// Отчество (терминология согласно XML)
    QString middleName;
    /// ФИО
    QString fullName;
    /// Должность
    QString function;
    qulonglong salary;

  private:
    Employee(QString surname, QString name, QString middle_name,
             QString function, qulonglong salary);
};

class Department : public ModelItem, public QEnableSharedFromThis<Department> {
    Q_OBJECT

  public:
    enum class Change {
        NAME_CHANGED,
        SUBDEPARTMENT_ADDED,
        SUBDEPARTMENT_REMOVED,
    };

    /**
     * @brief Создать новое подразделение
     * @param name название подразделения, не может быть пустым
     * @param parent подразделение, в котором будет новое, или nullptr
     *      Если не nullptr, автоматически вызывается parent.addSubdepartment
     */
    static QSharedPointer<Department>
    create(QString name, QSharedPointer<Department> department = nullptr);

    Type getType() const override;

    /// Вернуть название
    QString getName() const;

    /// Вернуть количество субподразделений
    size_t getNumSubdepartments() const;

    /// Вернуть указанное субподразделение (0 <= index < getNumSubdepartments())
    /// Индекс может быть недействительным после изменения объекта.
    /// @throw std::out_of_range если индекс недействительный
    QSharedPointer<Department> getSubdepartment(size_t index) const;

    /// Вернуть субподразделение с указанным именем или nullptr если отсутствует
    /// @throw std::invalid_argument если имя пустое
    QSharedPointer<Department> getSubdepartment(QString name) const;

    /// Вернуть количество сотрудников
    size_t getNumEmployees() const;

    /// Вернуть указанного сотрудника (0 <= idx < getNumEmployees())
    /// Индекс может быть недействительным после изменения объекта.
    /// @throw std::out_of_range если индекс недействительный
    QSharedPointer<const Employee> getEmployee(size_t index) const;

    /// Вернуть количество сотрудников, включая все субподразделения
    size_t getNumEmployeesWithSubdepartments() const;

    /// Вернуть сумму зарплат сотрудников в копейках
    qulonglong getTotalSalary() const;

    /// Вернуть сумму зарплат сотрудников в копейках, включая все
    /// субподразделения
    qulonglong getTotalSalaryWithSubdepartments() const;

    /// Вернуть среднюю зарплату сотрудника в копейках (округлённую вниз)
    qulonglong getMeanSalary() const;

    /// Вернуть среднюю зарплату сотрудника в копейках (округлённую вниз),
    /// считая всех сотрудников в субподразделениях
    qulonglong getMeanSalaryWithSubdepartments() const;

    /**
     * @brief Переименовать подразделение
     *
     * При успехе подаётся сигнал departmentsChanged. Никаких сигналов не
     * подаётся, если новое имя такое же, как старое.
     *
     * @param new_name новое имя
     * @return старое имя
     * @throw std::invalid_argument если new_name пустое или если parent
     *     установлен и в нём уже есть такой
     */
    QString rename(QString new_name);

    /**
     * @brief Добавить нового сотрудника
     *
     * При успешном добавлении зарплаты пересчитываются и подаются сигналы
     * employeesChanged, modelRecalculated.
     *
     * @param new_employee сотрудник, который не привязан ни к какому отделу
     * @throw std::invalid_argument если сотрудник привязан к какому-либо отделу
     */
    void addEmployee(QSharedPointer<const Employee> new_employee);

    /**
     * @brief Заменить сотрудника
     *
     * Смена данных сотрудника производится как замена одного на другого.
     * Старый сотрудник отвязывается от отдела, новый привязывается и ставится
     * в списках на место старого. При успешной замене зарплаты пересчитываются
     * и подаются сигналы employeesChanged, modelRecalculated.
     *
     * @param old_employee сотрудник, привязанный к этому отделу
     * @param new_employee сотрудник, который не привязан ни к какому отделу
     * @throw std::invalid_argument если параметры не соответствуют описанию
     */
    void replaceEmployee(QSharedPointer<const Employee> old_employee,
                         QSharedPointer<const Employee> new_employee);

    /**
     * @brief Отвязать сотрудника от подразделения
     *
     * При успехе зарплаты пересчитываются и подаются сигналы employeesChanged,
     * modelRecalculated.
     *
     * @param employee сотрудник, привязанный к этому отделу
     * @throw std::invalid_argument если сотрудник не привязан к этому отделу
     */
    void unassignEmployee(QSharedPointer<const Employee> employee);

    /**
     * @brief Добавить новое субподразделение
     *
     * При успешном добавлении зарплаты пересчитываются и подаются сигналы
     * departmentsChanged, modelRecalculated.
     *
     * @param new_department подразделение, не входящее в состав никакого
     * другого
     * @throw std::invalid_argument если подразделение уже входит в состав
     * другого
     */
    void addSubdepartment(QSharedPointer<Department> new_department);

    /**
     * @brief Вывести подразделение из состава данного
     *
     * При успехе зарплаты пересчитываются и подаются сигналы
     * departmentsChanged, modelRecalculated.
     *
     * @param department подразделение, входящее в состав данного
     * @throw std::invalid_argument если подразделение не входит в состав
     * данного
     */
    void unassignSubdepartment(QSharedPointer<Department> department);

    /// Вернуть true, если в модели нельзя посчитать средние значения
    /// ввиду выхода за пределы типов
    bool isModelInconsistent() const;

  signals:
    void employeesChanged(Department* where,
                          QSharedPointer<const Employee> removed_employee,
                          QSharedPointer<const Employee> new_employee);
    void departmentsChanged(Department* where, Change what, QString old_name,
                            QSharedPointer<Department> subdepartment);
    void modelRecalculated(Department* where);
    void inconsistentModel();

  private slots:
    void slotDepartmentsChanged(Department* where, Change what,
                                QString old_name,
                                QSharedPointer<Department> subdepartment);
    void slotModelRecalculated(Department* where);

  private:
    QString name;
    QVector<QSharedPointer<Department>> subdepartments{};
    QMap<QString, QSharedPointer<Department>> subdepartmentNames{};
    QVector<QSharedPointer<const Employee>> employees{};
    size_t totalEmployeesWithSubdeps{0};
    qulonglong totalSalary{0};
    qulonglong totalSalaryWithSubdeps{0};

    Department(QString name);
    void recalculateAfterChanges();

    bool inconsistentModelSignaled{false};
    /// Вернуть сумму аргументов. Если она превышает максимум, подать сигнал
    /// inconsistentModel и вернуть максимальное значение.
    template <class T> T safeadd(T x, T y);
};

class ModelItemPtr {
  public:
    /// Пустой конструктор создает объект, который не указывает ни на что
    ModelItemPtr();
    ModelItemPtr(QSharedPointer<Department> department);
    ModelItemPtr(QSharedPointer<const Employee> employee);

    /// Вернуть true если указатель не нулевой
    operator bool() const;

    /// Вернуть указатель, какой бы он ни был
    const ModelItem* get() const;
    QSharedPointer<Department> asDepartment() const;
    QSharedPointer<const Employee> asEmployee() const;

  private:
    // Если хранить оба, а не union, получится значительно меньше кода
    QSharedPointer<Department> department;
    QSharedPointer<const Employee> employee;
};
} // namespace TreeDemo

#endif // MODEL_H
