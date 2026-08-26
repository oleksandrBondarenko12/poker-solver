#ifndef STRATEGYEXPLORER_H
#define STRATEGYEXPLORER_H

#include <QDialog>
#include <QStyledItemDelegate>
#include <json.hpp>

// Forward declarations
class QTableWidget;
class QEvent;
class QTreeWidget;
class QTreeWidgetItem;
class QGridLayout;

class StrategyItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit StrategyItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class StrategyExplorer : public QDialog {
    Q_OBJECT

public:
    explicit StrategyExplorer(const nlohmann::json& strategy_data, QWidget *parent = nullptr);
    ~StrategyExplorer();

protected:
    // Declare the event filter override so it can be defined out-of-line.
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onNodeSelected(QTreeWidgetItem *item, int column);
    void onHandCellClicked(int row, int col);

private:
    void setupUI();
    void buildGameTreeUI(const nlohmann::json& node, QTreeWidgetItem* parent_item, const std::string& current_path = "");
    void updateStrategyGrid(const nlohmann::json& strategy_node);
    
    void clearLayout(QLayout* layout);

    // Member variable for the hand matrix
    QTableWidget *handMatrix;
    QTreeWidget *gameTreeWidget;
    
    QGridLayout *handStrategyLayout;
    QGridLayout *roughStrategyLayout;
    
    nlohmann::json strategy_data_;
    nlohmann::json current_node_; // Keep track of the currently selected node for combo lookup
};

#endif // STRATEGYEXPLORER_H 