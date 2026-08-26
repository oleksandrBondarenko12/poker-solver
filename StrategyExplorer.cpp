#include "StrategyExplorer.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QStringList>
#include <QDebug>
#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPainter>
#include <QVariant>
#include <QList>

void StrategyItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->save();
    
    QRect rect = option.rect;
    painter->fillRect(rect, QColor("#374151")); // Dark gray background by default

    QVariant freqsVar = index.data(Qt::UserRole + 1);
    QVariant colorsVar = index.data(Qt::UserRole + 2);
    
    if (freqsVar.isValid() && colorsVar.isValid()) {
        QList<QVariant> freqs = freqsVar.toList();
        QList<QVariant> colors = colorsVar.toList();
        
        if (freqs.size() == colors.size() && !freqs.isEmpty()) {
            double total_freq = 0;
            for (const QVariant& f : freqs) total_freq += f.toDouble();
            
            if (total_freq > 0.0001) {
                int current_x = rect.x();
                for (int i = 0; i < freqs.size(); ++i) {
                    double freq = freqs[i].toDouble();
                    if (freq <= 0) continue;
                    
                    int width = (freq / total_freq) * rect.width();
                    if (i == freqs.size() - 1) { // Fill remainder for last element to avoid gaps
                        width = rect.x() + rect.width() - current_x;
                    }
                    
                    QRect barRect(current_x, rect.y(), width, rect.height());
                    painter->fillRect(barRect, colors[i].value<QColor>());
                    current_x += width;
                }
            } else {
                painter->fillRect(rect, QColor("#4B5563")); // Base gray if no freq
            }
        }
    } else {
         QVariant bgVar = index.data(Qt::BackgroundRole);
         if (bgVar.isValid()) {
             painter->fillRect(rect, bgVar.value<QBrush>());
         }
    }

    QString text = index.data(Qt::DisplayRole).toString();
    if (!text.isEmpty()) {
        painter->setPen(QColor("#ffffff"));
        painter->drawText(rect, Qt::AlignCenter, text);
    }
    
    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(QColor("#ffffff"), 2));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect.adjusted(1, 1, -2, -2));
    }
    
    painter->restore();
}

void StrategyExplorer::clearLayout(QLayout* layout) {
    if (!layout) return;
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->layout()) {
            clearLayout(item->layout());
            delete item->layout();
        }
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

StrategyExplorer::StrategyExplorer(const nlohmann::json& strategy_data, QWidget *parent)
    : QDialog(parent), strategy_data_(strategy_data)
{
    setWindowTitle("Strategy Explorer");
    setMinimumSize(800, 600);
    setupUI();
}

StrategyExplorer::~StrategyExplorer() {}

void StrategyExplorer::setupUI() {
    // Main grid layout: 2 columns (left, right) similar to HTML grid-cols-[1fr,1.2fr]
    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->setHorizontalSpacing(20);
    mainLayout->setVerticalSpacing(20);
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(1, 12); // using 12 to simulate 1.2 fraction (scale factor)

    // ============================
    // LEFT COLUMN
    // ============================
    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(20);

    // --- Game Tree Panel ---
    QFrame *gameTreeFrame = new QFrame;
    gameTreeFrame->setFrameShape(QFrame::StyledPanel);
    gameTreeFrame->setStyleSheet("background-color: #374151; border-radius: 8px; padding: 8px;");
    QVBoxLayout *gameTreeLayout = new QVBoxLayout(gameTreeFrame);
    QLabel *gameTreeTitle = new QLabel("Game Tree");
    gameTreeTitle->setStyleSheet("font-size: 12px; margin-bottom: 8px;");
    gameTreeLayout->addWidget(gameTreeTitle);
    
    gameTreeWidget = new QTreeWidget;
    gameTreeWidget->setHeaderHidden(true);
    gameTreeWidget->setStyleSheet("QTreeWidget { background-color: #374151; color: #ffffff; border: none; } QTreeWidget::item:selected { background-color: #2563EB; }");
    
    // We pass "" as the initial path to refer to the root node
    buildGameTreeUI(strategy_data_, nullptr);
    
    connect(gameTreeWidget, &QTreeWidget::itemClicked, this, &StrategyExplorer::onNodeSelected);
    
    gameTreeLayout->addWidget(gameTreeWidget);
    leftLayout->addWidget(gameTreeFrame);

    // --- Card Selectors ---
    QWidget *cardSelectorsWidget = new QWidget;
    QGridLayout *cardSelectorsLayout = new QGridLayout(cardSelectorsWidget);
    cardSelectorsLayout->setHorizontalSpacing(10);
    cardSelectorsLayout->setVerticalSpacing(10);
    // Turn card
    QLabel *turnLabel = new QLabel("Turn card:");
    turnLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    cardSelectorsLayout->addWidget(turnLabel, 0, 0);
    QFrame *turnSelector = new QFrame;
    turnSelector->setFrameShape(QFrame::StyledPanel);
    turnSelector->setStyleSheet("background-color: #374151; border-radius: 4px; padding: 4px;");
    QHBoxLayout *turnSelectorLayout = new QHBoxLayout(turnSelector);
    turnSelectorLayout->setContentsMargins(8, 4, 8, 4);
    QLabel *turnCardLabel = new QLabel("2♠");
    QLabel *turnArrow = new QLabel("▼");
    turnArrow->setStyleSheet("color: #9CA3AF;");
    turnSelectorLayout->addWidget(turnCardLabel);
    turnSelectorLayout->addStretch();
    turnSelectorLayout->addWidget(turnArrow);
    cardSelectorsLayout->addWidget(turnSelector, 1, 0);
    // River card
    QLabel *riverLabel = new QLabel("River card:");
    riverLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    cardSelectorsLayout->addWidget(riverLabel, 0, 1);
    QFrame *riverSelector = new QFrame;
    riverSelector->setFrameShape(QFrame::StyledPanel);
    riverSelector->setStyleSheet("background-color: #374151; border-radius: 4px; padding: 4px;");
    QHBoxLayout *riverSelectorLayout = new QHBoxLayout(riverSelector);
    riverSelectorLayout->setContentsMargins(8, 4, 8, 4);
    QLabel *riverCardLabel = new QLabel("2♦");
    QLabel *riverArrow = new QLabel("▼");
    riverArrow->setStyleSheet("color: #9CA3AF;");
    riverSelectorLayout->addWidget(riverCardLabel);
    riverSelectorLayout->addStretch();
    riverSelectorLayout->addWidget(riverArrow);
    cardSelectorsLayout->addWidget(riverSelector, 1, 1);
    leftLayout->addWidget(cardSelectorsWidget);

    // --- Hand Matrix ---
    handMatrix = new QTableWidget(13, 13);
    handMatrix->horizontalHeader()->setVisible(false);
    handMatrix->verticalHeader()->setVisible(false);
    handMatrix->setEditTriggers(QAbstractItemView::NoEditTriggers);
    handMatrix->setSelectionMode(QAbstractItemView::NoSelection);
    handMatrix->setFocusPolicy(Qt::NoFocus);

    // Set overall table styling to match bg-gray-700 (#374151)
    handMatrix->setStyleSheet("QTableWidget { background-color: #374151; }");

    QStringList ranks = {"A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
    for (int row = 0; row < 13; ++row) {
        for (int col = 0; col < 13; ++col) {
            QString hand;
            if (row == col) {
                hand = ranks[row] + ranks[col];
            } else if (row < col) {
                hand = ranks[row] + ranks[col] + "s";
            } else {
                hand = ranks[col] + ranks[row] + "o";
            }
            QTableWidgetItem *item = new QTableWidgetItem(hand);
            item->setTextAlignment(Qt::AlignCenter);
            // Base background: bg-gray-600 (#4B5563)
            QColor bgColor = QColor("#4B5563");
            // Special cells get red-500 (#EF4444)
            if (hand == "22" || hand == "K2s") {
                bgColor = QColor("#EF4444");
            }
            item->setBackground(QBrush(bgColor));
            item->setForeground(QBrush(QColor("#ffffff")));
            handMatrix->setItem(row, col, item);
        }
    }

    // Instead of setting a fixed size (48 pixels), install an event filter
    // so that the cell size is computed based on the available width.
    handMatrix->installEventFilter(this);
    handMatrix->setItemDelegate(new StrategyItemDelegate(this));
    connect(handMatrix, &QTableWidget::cellClicked, this, &StrategyExplorer::onHandCellClicked);
    
    // Optionally, set a minimum size so the grid is not too small
    handMatrix->setMinimumWidth(300);

    leftLayout->addWidget(handMatrix);

    mainLayout->addWidget(leftWidget, 0, 0);

    // ============================
    // RIGHT COLUMN
    // ============================
    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setSpacing(20);

    // --- Hand Strategy Grid ---
    QWidget *handStrategyWidget = new QWidget;
    handStrategyLayout = new QGridLayout(handStrategyWidget);
    handStrategyLayout->setSpacing(5);
    // Create 4 strategy boxes (static examples)
    QStringList handNames = {"A♣7♣", "A♣7♥", "A♣7♦", "A♣7♠"};
    for (int i = 0; i < 4; ++i) {
        QFrame *strategyBox = new QFrame;
        strategyBox->setFrameShape(QFrame::StyledPanel);
        strategyBox->setStyleSheet("background-color: #10B981; border-radius: 4px; padding: 4px;");
        QVBoxLayout *boxLayout = new QVBoxLayout(strategyBox);
        QLabel *handLabel = new QLabel(handNames[i]);
        handLabel->setStyleSheet("margin-bottom: 4px; font-size: 10px;");
        boxLayout->addWidget(handLabel);
        boxLayout->addWidget(new QLabel("CHECK: 100.0%"));
        boxLayout->addWidget(new QLabel("BET 25: 0.0%"));
        boxLayout->addWidget(new QLabel("BET 200: 0.0%"));
        handStrategyLayout->addWidget(strategyBox, 0, i);
    }
    rightLayout->addWidget(handStrategyWidget);

    // --- Rough Strategy, Board Info & Range Controls ---
    QWidget *miscWidget = new QWidget;
    QVBoxLayout *miscLayout = new QVBoxLayout(miscWidget);
    miscLayout->setSpacing(10);

    // Rough Strategy Title
    QLabel *roughStrategyTitle = new QLabel("Rough Strategy");
    roughStrategyTitle->setStyleSheet("font-size: 12px; margin-bottom: 4px;");
    miscLayout->addWidget(roughStrategyTitle);

    // Rough Strategy Grid (3 cells)
    QWidget *roughStrategyWidget = new QWidget;
    roughStrategyLayout = new QGridLayout(roughStrategyWidget);
    roughStrategyLayout->setSpacing(5);
    {
        // Cell 1: CHECK
        QFrame *cell = new QFrame;
        cell->setFrameShape(QFrame::StyledPanel);
        cell->setStyleSheet("background-color: #10B981; padding: 4px; border-radius: 4px;");
        QVBoxLayout *cellLayout = new QVBoxLayout(cell);
        QLabel *cellTitle = new QLabel("CHECK");
        cellTitle->setStyleSheet("font-weight: bold;");
        cellLayout->addWidget(cellTitle);
        cellLayout->addWidget(new QLabel("98.9%"));
        cellLayout->addWidget(new QLabel("273.2 combos"));
        roughStrategyLayout->addWidget(cell, 0, 0);
    }
    {
        // Cell 2: BET 25.0
        QFrame *cell = new QFrame;
        cell->setFrameShape(QFrame::StyledPanel);
        cell->setStyleSheet("background-color: #F87171; padding: 4px; border-radius: 4px;");
        QVBoxLayout *cellLayout = new QVBoxLayout(cell);
        QLabel *cellTitle = new QLabel("BET 25.0");
        cellTitle->setStyleSheet("font-weight: bold;");
        cellLayout->addWidget(cellTitle);
        cellLayout->addWidget(new QLabel("1.1%"));
        cellLayout->addWidget(new QLabel("3.0 combos"));
        roughStrategyLayout->addWidget(cell, 0, 1);
    }
    {
        // Cell 3: BET 200.0
        QFrame *cell = new QFrame;
        cell->setFrameShape(QFrame::StyledPanel);
        cell->setStyleSheet("background-color: #F87171; padding: 4px; border-radius: 4px;");
        QVBoxLayout *cellLayout = new QVBoxLayout(cell);
        QLabel *cellTitle = new QLabel("BET 200.0");
        cellTitle->setStyleSheet("font-weight: bold;");
        cellLayout->addWidget(cellTitle);
        cellLayout->addWidget(new QLabel("0.0%"));
        cellLayout->addWidget(new QLabel("0.0 combos"));
        roughStrategyLayout->addWidget(cell, 0, 2);
    }
    miscLayout->addWidget(roughStrategyWidget);

    // Board Info
    QWidget *boardInfoWidget = new QWidget;
    QVBoxLayout *boardInfoLayout = new QVBoxLayout(boardInfoWidget);
    QLabel *boardLabel = new QLabel("board: Q♠ J♥ 2♥");
    boardLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    QLabel *decisionNodeLabel = new QLabel("OOP decision node");
    decisionNodeLabel->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    boardInfoLayout->addWidget(boardLabel);
    boardInfoLayout->addWidget(decisionNodeLabel);
    miscLayout->addWidget(boardInfoWidget);

    // Range & Strategy Controls
    QWidget *rangeStrategyWidget = new QWidget;
    QGridLayout *rangeStrategyLayout = new QGridLayout(rangeStrategyWidget);
    rangeStrategyLayout->setSpacing(10);
    {
        // Left: Range controls
        QVBoxLayout *leftRangeLayout = new QVBoxLayout;
        QLabel *rangeTitle = new QLabel("Range:");
        rangeTitle->setStyleSheet("color: #9CA3AF; font-size: 12px; margin-bottom: 4px;");
        leftRangeLayout->addWidget(rangeTitle);
        QPushButton *ipButton = new QPushButton("IP");
        QPushButton *oopButton = new QPushButton("OOP");
        ipButton->setStyleSheet("background-color: #374151; border-radius: 4px; padding: 4px;");
        oopButton->setStyleSheet("background-color: #374151; border-radius: 4px; padding: 4px;");
        leftRangeLayout->addWidget(ipButton);
        leftRangeLayout->addWidget(oopButton);
        rangeStrategyLayout->addLayout(leftRangeLayout, 0, 0);
    }
    {
        // Right: Strategy & EV controls
        QVBoxLayout *rightStrategyLayout = new QVBoxLayout;
        QLabel *strategyTitle = new QLabel("Strategy & EVs:");
        strategyTitle->setStyleSheet("color: #9CA3AF; font-size: 12px; margin-bottom: 4px;");
        rightStrategyLayout->addWidget(strategyTitle);
        QPushButton *strategyButton = new QPushButton("strategy");
        QPushButton *evStrategyButton = new QPushButton("EV + strategy");
        QPushButton *evButton = new QPushButton("EV");
        strategyButton->setStyleSheet("background-color: #374151; border-radius: 4px; padding: 4px;");
        evStrategyButton->setStyleSheet("background-color: #374151; border-radius: 4px; padding: 4px;");
        evButton->setStyleSheet("background-color: #374151; border-radius: 4px; padding: 4px;");
        rightStrategyLayout->addWidget(strategyButton);
        rightStrategyLayout->addWidget(evStrategyButton);
        rightStrategyLayout->addWidget(evButton);
        rangeStrategyLayout->addLayout(rightStrategyLayout, 0, 1);
    }
    miscLayout->addWidget(rangeStrategyWidget);

    rightLayout->addWidget(miscWidget);

    mainLayout->addWidget(rightWidget, 0, 1);

    setLayout(mainLayout);
}

bool StrategyExplorer::eventFilter(QObject *obj, QEvent *event)
{
    // If the event is a resize event on the hand matrix, recalc cell sizes
    if (obj == handMatrix && event->type() == QEvent::Resize) {
        // Get available width from the viewport (cells area)
        int newWidth = handMatrix->viewport()->width();
        // Compute cell size by dividing the available width by 13 columns
        int cellSize = newWidth / 13;
        // Apply new size to each column and row
        for (int i = 0; i < 13; ++i) {
            handMatrix->setColumnWidth(i, cellSize);
            handMatrix->setRowHeight(i, cellSize);
        }
    }
    // Pass the event on to the base class
    return QDialog::eventFilter(obj, event);
}

void StrategyExplorer::buildGameTreeUI(const nlohmann::json& node, QTreeWidgetItem* parent_item, const std::string& current_path) {
    if (node.is_null()) return;

    QTreeWidgetItem* item = new QTreeWidgetItem;
    
    QString label;
    if (node.contains("node_type")) {
        std::string type = node["node_type"].get<std::string>();
        if (type == "Action") {
            if (node.contains("action_label")) {
                std::string action = node["action_label"].get<std::string>();
                if (action == "c") label = "CHECK / CALL";
                else if (action == "f") label = "FOLD";
                else if (action.find("b") == 0) label = "BET " + QString::fromStdString(action.substr(2));
                else label = QString::fromStdString(action);
            } else {
                label = "Decision Node";
            }
        } else if (type == "Chance") {
            label = "Chance Node";
        } else if (type == "Terminal") {
            label = "Terminal Node";
        } else if (type == "Showdown") {
            label = "Showdown Node";
        }
    }

    if (label.isEmpty()) {
        label = "ROOT Node";
    }

    item->setText(0, label);
    item->setData(0, Qt::UserRole, QString::fromStdString(current_path));
    
    if (parent_item) {
        parent_item->addChild(item);
    } else {
        gameTreeWidget->addTopLevelItem(item);
    }

    // Recursively add children
    if (node.contains("children") && node["children"].is_array()) {
        const auto& children = node["children"];
        for (size_t i = 0; i < children.size(); ++i) {
            const auto& child = children[i];
            if (child.contains("node") && !child["node"].is_null()) {
                std::string child_path = current_path + "/children/" + std::to_string(i) + "/node";
                // Inject action label for UI
                nlohmann::json child_node = child["node"];
                if (child.contains("action")) {
                    child_node["action_label"] = child["action"];
                }
                buildGameTreeUI(child_node, item, child_path);
            }
        }
    } else if (node.contains("child") && !node["child"].is_null()) {
        std::string child_path = current_path + "/child";
        buildGameTreeUI(node["child"], item, child_path);
    }
}

void StrategyExplorer::onNodeSelected(QTreeWidgetItem *item, int column) {
    QString path = item->data(0, Qt::UserRole).toString();
    try {
        nlohmann::json node;
        if (path.isEmpty()) {
            node = strategy_data_;
        } else {
            node = strategy_data_.at(nlohmann::json::json_pointer(path.toStdString()));
        }
        updateStrategyGrid(node);
    } catch (const std::exception& e) {
        qDebug() << "Error retrieving node at path" << path << ":" << e.what();
    }
}

void StrategyExplorer::onHandCellClicked(int row, int col) {
    QTableWidgetItem *item = handMatrix->item(row, col);
    if (!item) return;
    
    QString category = item->text();
    if (category.isEmpty()) return;
    
    clearLayout(handStrategyLayout);
    
    if (current_node_.is_null() || !current_node_.contains("strategy")) return;
    auto strategy = current_node_["strategy"];
    if (!strategy.contains("strategy") || !strategy["strategy"].is_array()) return;
    const auto& strat_matrix = strategy["strategy"];
    
    int player = current_node_.contains("player") ? current_node_["player"].get<int>() : 0;
    if (!strategy_data_.contains("ranges") || player >= strategy_data_["ranges"].size()) return;
    const auto& player_range = strategy_data_["ranges"][player];
    
    QString ranks = "AKQJT98765432";
    
    QStringList action_names;
    QList<QColor> action_colors;
    if (current_node_.contains("children") && current_node_["children"].is_array()) {
        for (const auto& child : current_node_["children"]) {
            std::string action = child["action"].get<std::string>();
            action_names.append(QString::fromStdString(action));
            if (action == "c") action_colors.append(QColor("#22C55E"));
            else if (action == "f") action_colors.append(QColor("#3B82F6"));
            else action_colors.append(QColor("#EF4444"));
        }
    }
    
    int box_index = 0;
    for (size_t h = 0; h < player_range.size(); ++h) {
        if (h >= strat_matrix.size()) break;
        
        QString hand_str = QString::fromStdString(player_range[h].get<std::string>());
        if (hand_str.length() < 4) continue;
        
        QString rank1 = hand_str.mid(0, 1);
        QString suit1 = hand_str.mid(1, 1);
        QString rank2 = hand_str.mid(2, 1);
        QString suit2 = hand_str.mid(3, 1);
        
        int r1 = ranks.indexOf(rank1);
        int r2 = ranks.indexOf(rank2);
        if (r1 > r2) {
           std::swap(r1, r2);
           std::swap(rank1, rank2);
           std::swap(suit1, suit2);
        }
        
        QString combo_category;
        if (r1 == r2) combo_category = rank1 + rank2;
        else if (suit1 == suit2) combo_category = rank1 + rank2 + "s";
        else combo_category = rank1 + rank2 + "o";
        
        if (combo_category == category) {
            QString nice_hand_name = rank1;
            if (suit1 == "c") nice_hand_name += "♣";
            else if (suit1 == "d") nice_hand_name += "♦";
            else if (suit1 == "h") nice_hand_name += "♥";
            else if (suit1 == "s") nice_hand_name += "♠";
            nice_hand_name += rank2;
            if (suit2 == "c") nice_hand_name += "♣";
            else if (suit2 == "d") nice_hand_name += "♦";
            else if (suit2 == "h") nice_hand_name += "♥";
            else if (suit2 == "s") nice_hand_name += "♠";
                                     
            QFrame *strategyBox = new QFrame;
            strategyBox->setFrameShape(QFrame::StyledPanel);
            strategyBox->setStyleSheet("background-color: #374151; border-radius: 4px; padding: 4px; border: 1px solid #4B5563;");
            QVBoxLayout *boxLayout = new QVBoxLayout(strategyBox);
            boxLayout->setSpacing(2);
            boxLayout->setContentsMargins(4, 4, 4, 4);
            
            QLabel *handLabel = new QLabel(nice_hand_name);
            handLabel->setStyleSheet("margin-bottom: 2px; font-weight: bold; font-size: 11px; color: #ffffff;");
            boxLayout->addWidget(handLabel);
            
            const auto& hand_strats = strat_matrix[h];
            for (size_t a = 0; a < hand_strats.size() && a < action_names.size(); ++a) {
                double pct = hand_strats[a].get<double>() * 100.0;
                
                QString actLabel = action_names[a];
                if (actLabel == "c") actLabel = "CHECK";
                else if (actLabel == "f") actLabel = "FOLD";
                else if (actLabel.startsWith("b")) actLabel = "BET " + actLabel.mid(2);
                
                QLabel *actInfo = new QLabel(QString("%1: %2%").arg(actLabel).arg(pct, 0, 'f', 1));
                actInfo->setStyleSheet(QString("color: %1; font-size: 10px;").arg(action_colors[a].name()));
                boxLayout->addWidget(actInfo);
            }
            
            handStrategyLayout->addWidget(strategyBox, box_index / 6, box_index % 6);
            box_index++;
        }
    }
}

void StrategyExplorer::updateStrategyGrid(const nlohmann::json& node) {
    current_node_ = node;
    if (!node.contains("strategy") || node["strategy"].is_null()) {
        return; // Empty grid if no strategy
    }

    auto strategy = node["strategy"];
    if (!strategy.contains("strategy") || !strategy["strategy"].is_array()) {
        return;
    }
    const auto& strat_matrix = strategy["strategy"];
    
    bool has_evs = strategy.contains("evs") && strategy["evs"].is_array();
    const auto& ev_matrix = has_evs ? strategy["evs"] : nlohmann::json();

    int player = node.contains("player") ? node["player"].get<int>() : 0;
    if (!strategy_data_.contains("ranges") || !strategy_data_["ranges"].is_array() || player >= strategy_data_["ranges"].size()) {
        return;
    }

    const auto& player_range = strategy_data_["ranges"][player];
    
    QStringList action_names;
    QList<QVariant> action_colors;
    if (node.contains("children") && node["children"].is_array()) {
        for (const auto& child : node["children"]) {
            std::string action = child["action"].get<std::string>();
            action_names.append(QString::fromStdString(action));
            if (action == "c") action_colors.append(QColor("#22C55E"));
            else if (action == "f") action_colors.append(QColor("#3B82F6"));
            else action_colors.append(QColor("#EF4444"));
        }
    }
    
    struct HandData {
        double total_ev = 0;
        int count = 0;
        std::vector<double> action_freqs;
    };
    std::map<QString, HandData> category_data;

    QString ranks = "AKQJT98765432";
    std::vector<double> total_action_freqs(action_names.size(), 0.0);
    double total_combos = 0;

    for (size_t h = 0; h < player_range.size(); ++h) {
        if (h >= strat_matrix.size()) break;
        
        QString hand_str = QString::fromStdString(player_range[h].get<std::string>());
        if (hand_str.length() < 4) continue;
        
        QString rank1 = hand_str.mid(0, 1);
        QString suit1 = hand_str.mid(1, 1);
        QString rank2 = hand_str.mid(2, 1);
        QString suit2 = hand_str.mid(3, 1);
        
        int r1 = ranks.indexOf(rank1);
        int r2 = ranks.indexOf(rank2);
        if (r1 > r2) {
           std::swap(r1, r2);
           std::swap(rank1, rank2);
           std::swap(suit1, suit2);
        }
        
        QString category;
        if (r1 == r2) {
            category = rank1 + rank2;
        } else if (suit1 == suit2) {
            category = rank1 + rank2 + "s";
        } else {
            category = rank1 + rank2 + "o";
        }

        HandData& cd = category_data[category];
        cd.count++;

        if (has_evs && h < ev_matrix.size() && ev_matrix[h].is_array() && ev_matrix[h].size() > 0) {
            double max_ev = -999999;
            for (size_t a = 0; a < ev_matrix[h].size(); ++a) {
                if (ev_matrix[h][a].is_null()) continue;
                double val = ev_matrix[h][a].get<double>();
                if (val > max_ev) max_ev = val;
            }
            if (max_ev > -999999) cd.total_ev += max_ev;
        }

        const auto& hand_strats = strat_matrix[h];
        if (cd.action_freqs.empty()) {
            cd.action_freqs.resize(hand_strats.size(), 0.0);
        }
        for (size_t a = 0; a < hand_strats.size(); ++a) {
            double f = hand_strats[a].get<double>();
            cd.action_freqs[a] += f;
            if (a < total_action_freqs.size()) {
                total_action_freqs[a] += f;
            }
        }
        total_combos += 1.0;
    }

    for (int row = 0; row < 13; ++row) {
        for (int col = 0; col < 13; ++col) {
            QString category;
            if (row == col) {
                category = QString(ranks[row]) + ranks[col];
            } else if (row < col) {
                category = QString(ranks[row]) + ranks[col] + "s";
            } else {
                category = QString(ranks[col]) + ranks[row] + "o";
            }

            QTableWidgetItem *item = handMatrix->item(row, col);
            if (!item) continue;

            if (category_data.contains(category)) {
                HandData& cd = category_data[category];
                QList<QVariant> freqs;
                for (double f : cd.action_freqs) {
                    freqs.append(f / cd.count); // Average probability across suits
                }
                
                item->setText(category);
                item->setData(Qt::UserRole + 1, freqs);
                item->setData(Qt::UserRole + 2, action_colors);
            } else {
                item->setText(category);
                item->setData(Qt::UserRole + 1, QVariant()); // clear freqs
                item->setData(Qt::UserRole + 2, QVariant()); // clear colors
            }
        }
    }
    
    // Update Rough Strategy panel dynamically
    clearLayout(roughStrategyLayout);
    if (total_combos > 0) {
        for (int a = 0; a < action_names.size(); ++a) {
            double pct = (total_action_freqs[a] / total_combos) * 100.0;
            double combos = total_action_freqs[a];
            
            QFrame *cell = new QFrame;
            cell->setFrameShape(QFrame::StyledPanel);
            
            QString actionLabel = action_names[a];
            if (actionLabel == "c") actionLabel = "CHECK/CALL";
            else if (actionLabel == "f") actionLabel = "FOLD";
            else if (actionLabel.startsWith("b")) actionLabel = "BET " + actionLabel.mid(2);
            
            QColor color = action_colors[a].value<QColor>();
            QString bg_style = QString("background-color: %1; padding: 4px; border-radius: 4px; color: #ffffff;")
                                .arg(color.name());
            cell->setStyleSheet(bg_style);
            
            QVBoxLayout *cellLayout = new QVBoxLayout(cell);
            QLabel *cellTitle = new QLabel(actionLabel);
            cellTitle->setStyleSheet("font-weight: bold;");
            cellLayout->addWidget(cellTitle);
            cellLayout->addWidget(new QLabel(QString::number(pct, 'f', 1) + "%"));
            cellLayout->addWidget(new QLabel(QString::number(combos, 'f', 1) + " combos"));
            
            roughStrategyLayout->addWidget(cell, 0, a);
        }
    }
} 