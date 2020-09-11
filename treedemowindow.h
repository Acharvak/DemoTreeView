#ifndef TREEDEMOWINDOW_H
#define TREEDEMOWINDOW_H

#include "controller.h"
#include "model.h"

#include <QMainWindow>
#include <QMap>
#include <QSharedPointer>
#include <QTreeWidgetItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class TreeDemoWindow;
}
QT_END_NAMESPACE

using namespace TreeDemo;

class TreeDemoWindow : public QMainWindow {
    Q_OBJECT

  public:
    TreeDemoWindow(QWidget* parent = nullptr);
    TreeDemoWindow(TreeDemoWindow&) = delete;
    TreeDemoWindow(TreeDemoWindow&&) = delete;
    ~TreeDemoWindow();

    void loadXMLFile(QString path);
    void setModel(QSharedPointer<Department> root);
    void saveXMLFile(QString path);

  private slots:
    void VE_itemSelectionChanged();
    void VE_loadButtonClicked(bool checked);
    void VE_saveButtonClicked(bool checked);
    void VE_subdepartmentsCheckboxClicked(bool checked);
    void VE_editButtonClicked(bool checked);
    void VE_deleteButtonClicked(bool checked);
    void VE_newEmployeeButtonClicked(bool checked);
    void VE_newDepartmentButtonClicked(bool checked);
    void VE_undoButtonClicked(bool checked);
    void VE_redoButtonClicked(bool checked);

    void ME_employeesChanged(Department* where,
                             QSharedPointer<const Employee> removed_employee,
                             QSharedPointer<const Employee> new_employee);
    void ME_departmentsChanged(Department* where, Department::Change what,
                               QString old_name,
                               QSharedPointer<Department> subdepartment);
    void ME_modelRecalculated(Department* where);
    void ME_inconsistentModel();

  private:
    enum class SelectionType {
        NONE,
        DEPARTMENT,
        EMPLOYEE,
        SEVERAL_EMPLOYEES,
        INCOHERENT
    };

    static QString getFixed100(qulonglong value);

    /**
     * @brief Десериализировать XML-элемент <employment>
     *
     * Зарплата из XML читается в рублях.
     *
     * @throw std::invalid_argument из create или если зарплата не число или < 0
     */
    static QSharedPointer<const Employee> deserialize(QDomElement employment);

    /**
     * @brief Десериализовать XML-элемент <department>
     * @throw std::invalid_argument если возникнет проблема при десериализации
     */
    static QSharedPointer<Department>
    deserialize(QDomElement department, QSharedPointer<Department> parent);

    Ui::TreeDemoWindow* ui;
    QString savePathHint{};
    QSharedPointer<Department> modelRoot{nullptr};
    Controller controller{};
    QMap<QTreeWidgetItem*, ModelItemPtr> tree2model{};
    QMap<const ModelItem*, QTreeWidgetItem*> model2tree{};
    QVector<const ModelItem*> cutItems{};

    SelectionType getSelectionType(const QList<QTreeWidgetItem*> selection);
    void resetUndoRedo();
    ModelItemPtr translateTree2Model(QTreeWidgetItem* twitem);
    QTreeWidgetItem* translateModel2Tree(const ModelItem* mitem);
    void addDepartmentView(QSharedPointer<Department> department,
                           QTreeWidgetItem* parent_view = nullptr);
    void updateDepartmentView(const Department* department,
                              QTreeWidgetItem* view = nullptr);
    // Не const Department* потому что надо отсоединять сигналы
    void removeDepartmentView(Department* department,
                              QTreeWidgetItem* view = nullptr,
                              bool delete_view = true);
    void addEmployeeView(QSharedPointer<const Employee> employee,
                         QTreeWidgetItem* parent_view = nullptr);
    void updateEmployeeView(const Employee* employee,
                            QTreeWidgetItem* view = nullptr);
    void removeEmployeeView(const Employee* employee, QTreeWidgetItem* view,
                            bool actually_remove = true);

    /// Сериализовать подразделение в данный элемент, в порядке представления
    void serializeDepartment(QDomDocument& document, QDomNode& target,
                             QTreeWidgetItem* twitem);
};
#endif // TREEDEMOWINDOW_H
