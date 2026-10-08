#ifndef STRATEGYEXPLORER_H
#define STRATEGYEXPLORER_H

#include <QDialog>
#include <QStyledItemDelegate>
#include "poker_solver/json.hpp"
#include <vector>
#include <string>

// Forward declarations
class QTableWidget;
class QEvent;
class QTreeWidget;
class QTreeWidgetItem;
class QGridLayout;
class QLabel;
class QPushButton;
class QFrame;

enum class ViewMode {
    kStrategy,
    kEv,
    kStrategyAndEv
};

class StrategyItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit StrategyItemDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent), view_mode_(ViewMode::kStrategy) {}

    void setViewMode(ViewMode mode) { view_mode_ = mode; }
    ViewMode getViewMode() const { return view_mode_; }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    ViewMode view_mode_;
};

class StrategyExplorer : public QDialog {
    Q_OBJECT

public:
    explicit StrategyExplorer(const nlohmann::json& strategy_data, QWidget *parent = nullptr);
    ~StrategyExplorer();

    static QColor getActionColor(const QString& action_name, int action_index, int total_actions);
    static QString formatCardHtml(const QString& card_str);
    static QString formatCardPlain(const QString& card_str);
    bool verifyChanceNodeStrategies();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onNodeSelected(QTreeWidgetItem *item, int column);
    void onHandCellClicked(int row, int col);
    void onViewModeChanged(ViewMode mode);
    void onPlayerRangeToggled(int player);
    void onItemExpanded(QTreeWidgetItem *item);

private:
    void setupUI();
    void buildGameTreeUI(const nlohmann::json& node, QTreeWidgetItem* parent_item, const std::string& current_path = "", const QString& action_label = "");
    void populateImmediateChildren(QTreeWidgetItem* parent_item, const nlohmann::json& node, const std::string& current_path);
    void updateHeader(const nlohmann::json& node);
    void updateStrategyGrid(const nlohmann::json& strategy_node);
    void clearLayout(QLayout* layout);

    // UI Elements
    QTreeWidget *gameTreeWidget;
    QTableWidget *handMatrix;
    StrategyItemDelegate *matrixDelegate;

    // Header Widgets
    QLabel *boardCardsLabel;
    QLabel *nodeInfoLabel;
    QLabel *potLabel;
    QLabel *selectedHandLabel;

    // Control Buttons
    QPushButton *btnStrategy;
    QPushButton *btnEvStrategy;
    QPushButton *btnEv;
    QPushButton *btnPlayerIP;
    QPushButton *btnPlayerOOP;

    // Dynamic Strategy Layouts
    QGridLayout *handStrategyLayout;
    QGridLayout *roughStrategyLayout;

    // State
    nlohmann::json strategy_data_;
    nlohmann::json current_node_;
    ViewMode current_view_mode_ = ViewMode::kStrategy;
    int current_display_player_ = -1; // -1 means default to node acting player
    int selected_hand_row_ = -1;
    int selected_hand_col_ = -1;
    std::vector<std::string> initial_board_;
};

#endif // STRATEGYEXPLORER_H 