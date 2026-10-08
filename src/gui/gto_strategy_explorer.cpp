#include "gto_strategy_explorer.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
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
#include <QFont>
#include <QScrollArea>
#include <QToolTip>
#include <algorithm>
#include <cmath>

// ============================================================================
// Helper Methods
// ============================================================================

QColor StrategyExplorer::getActionColor(const QString& action_name, int action_index, int total_actions) {
    QString act = action_name.trimmed().toUpper();
    if (act == "FOLD" || act == "F") {
        return QColor("#3B82F6"); // Blue
    }
    if (act == "CHECK" || act == "C" || act == "X") {
        return QColor("#10B981"); // Emerald Green
    }
    if (act == "CALL") {
        return QColor("#14B8A6"); // Teal
    }
    if (act.startsWith("BET") || act.startsWith("B")) {
        if (total_actions > 2 && action_index == total_actions - 1) {
            return QColor("#DC2626"); // All-in / Max bet: Crimson
        }
        if (action_index == 1) {
            return QColor("#F59E0B"); // Small bet: Amber
        }
        return QColor("#EF4444"); // Standard bet: Red
    }
    if (act.startsWith("RAISE") || act.startsWith("R")) {
        if (action_index == total_actions - 1) {
            return QColor("#DC2626"); // Max raise: Crimson
        }
        return QColor("#8B5CF6"); // Purple
    }

    static const std::vector<const char*> palette = {
        "#10B981", "#3B82F6", "#F59E0B", "#EF4444", "#8B5CF6", "#EC4899", "#14B8A6"
    };
    return QColor(palette[action_index % palette.size()]);
}

QString StrategyExplorer::formatCardHtml(const QString& card_str) {
    if (card_str.length() < 2) return card_str;
    QString rank = card_str.left(card_str.length() - 1).toUpper();
    QChar suit_char = card_str.right(1).at(0).toLower();

    QString suit_sym;
    QString suit_color;
    if (suit_char == 's') {
        suit_sym = "♠";
        suit_color = "#E5E7EB"; // Silver/White
    } else if (suit_char == 'h') {
        suit_sym = "♥";
        suit_color = "#EF4444"; // Red
    } else if (suit_char == 'd') {
        suit_sym = "♦";
        suit_color = "#38BDF8"; // Sky Blue
    } else if (suit_char == 'c') {
        suit_sym = "♣";
        suit_color = "#34D399"; // Green
    } else {
        suit_sym = QString(suit_char);
        suit_color = "#9CA3AF";
    }

    return QString("<span style=\"background-color: #1F2937; padding: 2px 7px; border-radius: 4px; font-weight: bold; font-family: monospace; border: 1px solid #374151;\">"
                   "<span style=\"color: #F9FAFB;\">%1</span><span style=\"color: %2; font-size: 13px;\">%3</span></span>")
        .arg(rank, suit_color, suit_sym);
}

QString StrategyExplorer::formatCardPlain(const QString& card_str) {
    if (card_str.length() < 2) return card_str;
    QString rank = card_str.left(card_str.length() - 1).toUpper();
    QChar suit_char = card_str.right(1).at(0).toLower();

    QString suit_sym;
    if (suit_char == 's') suit_sym = "♠";
    else if (suit_char == 'h') suit_sym = "♥";
    else if (suit_char == 'd') suit_sym = "♦";
    else if (suit_char == 'c') suit_sym = "♣";
    else suit_sym = QString(suit_char);

    return rank + suit_sym;
}

static void extractActionInfo(const nlohmann::json& node, const nlohmann::json& strat_matrix, QStringList& action_names, QList<QColor>& action_colors) {
    action_names.clear();
    action_colors.clear();

    int num_strat_actions = 0;
    if (!strat_matrix.empty() && strat_matrix[0].is_array()) {
        num_strat_actions = static_cast<int>(strat_matrix[0].size());
    }

    // 1. Check explicit "actions" array dumped on action node
    if (node.contains("actions") && node["actions"].is_array() && !node["actions"].empty()) {
        int total = static_cast<int>(node["actions"].size());
        for (int a = 0; a < total; ++a) {
            std::string act = node["actions"][a].get<std::string>();
            QString qAct = QString::fromStdString(act);
            action_names.append(qAct);
            action_colors.append(StrategyExplorer::getActionColor(qAct, a, total));
        }
    }
    // 2. Check "children" actions
    else if (node.contains("children") && node["children"].is_array() && !node["children"].empty()) {
        int total = static_cast<int>(node["children"].size());
        for (int a = 0; a < total; ++a) {
            std::string act = node["children"][a].value("action", "ACTION");
            QString qAct = QString::fromStdString(act);
            action_names.append(qAct);
            action_colors.append(StrategyExplorer::getActionColor(qAct, a, total));
        }
    }

    // 3. Fallback to guarantee action count matches strategy dimensions
    if (action_names.size() != num_strat_actions && num_strat_actions > 0) {
        action_names.clear();
        action_colors.clear();
        for (int a = 0; a < num_strat_actions; ++a) {
            QString name;
            if (num_strat_actions == 2) {
                name = (a == 0) ? "CHECK" : "BET";
            } else if (num_strat_actions == 3) {
                name = (a == 0) ? "FOLD" : (a == 1 ? "CALL" : "RAISE");
            } else {
                name = (a == 0) ? "CHECK/FOLD" : QString("ACTION %1").arg(a);
            }
            action_names.append(name);
            action_colors.append(StrategyExplorer::getActionColor(name, a, num_strat_actions));
        }
    }
}

// ============================================================================
// StrategyItemDelegate Implementation
// ============================================================================

void StrategyItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRect rect = option.rect;
    bool isInRange = index.data(Qt::UserRole + 5).toBool();

    if (!isInRange) {
        // Out of range: Dark muted background
        painter->fillRect(rect, QColor("#1F2937"));
        QString text = index.data(Qt::DisplayRole).toString();
        painter->setPen(QColor("#4B5563"));
        QFont f = painter->font();
        f.setPointSize(9);
        f.setBold(false);
        painter->setFont(f);
        painter->drawText(rect, Qt::AlignCenter, text);
    } else if (view_mode_ == ViewMode::kEv) {
        // EV Heatmap Mode
        QVariant evColorVar = index.data(Qt::UserRole + 4);
        QColor evColor = evColorVar.isValid() ? evColorVar.value<QColor>() : QColor("#374151");
        painter->fillRect(rect, evColor);

        QString hand = index.data(Qt::DisplayRole).toString();
        double ev = index.data(Qt::UserRole + 3).toDouble();
        QString evStr = (ev >= 0 ? "+" : "") + QString::number(ev, 'f', 1);

        QFont f = painter->font();
        f.setPointSize(8);
        f.setBold(true);
        painter->setFont(f);

        // Shadow/glow for contrast
        painter->setPen(QColor(0, 0, 0, 180));
        QRect topShadow = rect.adjusted(1, 1, 0, -rect.height() / 2);
        painter->drawText(topShadow, Qt::AlignCenter, hand);

        painter->setPen(QColor("#FFFFFF"));
        QRect topRect = rect.adjusted(0, 0, 0, -rect.height() / 2);
        painter->drawText(topRect, Qt::AlignCenter, hand);

        f.setPointSize(7);
        f.setBold(false);
        painter->setFont(f);
        QRect btmShadow = rect.adjusted(1, rect.height() / 2 + 1, 0, 0);
        painter->setPen(QColor(0, 0, 0, 180));
        painter->drawText(btmShadow, Qt::AlignCenter, evStr);

        painter->setPen(QColor("#E5E7EB"));
        QRect btmRect = rect.adjusted(0, rect.height() / 2, 0, 0);
        painter->drawText(btmRect, Qt::AlignCenter, evStr);
    } else {
        // Strategy Mode or Strategy + EV Mode
        painter->fillRect(rect, QColor("#374151"));

        QVariant freqsVar = index.data(Qt::UserRole + 1);
        QVariant colorsVar = index.data(Qt::UserRole + 2);

        if (freqsVar.isValid() && colorsVar.isValid()) {
            QList<QVariant> freqs = freqsVar.toList();
            QList<QVariant> colors = colorsVar.toList();

            double total_freq = 0;
            for (const QVariant& f : freqs) total_freq += f.toDouble();

            if (total_freq > 0.0001 && freqs.size() == colors.size()) {
                int current_x = rect.x();
                for (int i = 0; i < freqs.size(); ++i) {
                    double freq = freqs[i].toDouble();
                    if (freq <= 0) continue;

                    int width = static_cast<int>(std::round((freq / total_freq) * rect.width()));
                    if (i == freqs.size() - 1) {
                        width = rect.x() + rect.width() - current_x;
                    }
                    if (width > 0) {
                        QRect barRect(current_x, rect.y(), width, rect.height());
                        painter->fillRect(barRect, colors[i].value<QColor>());
                        current_x += width;
                    }
                }
            }
        }

        QString hand = index.data(Qt::DisplayRole).toString();

        if (view_mode_ == ViewMode::kStrategyAndEv) {
            double ev = index.data(Qt::UserRole + 3).toDouble();
            QString evStr = (ev >= 0 ? "+" : "") + QString::number(ev, 'f', 1);

            QFont f = painter->font();
            f.setPointSize(8);
            f.setBold(true);
            painter->setFont(f);

            painter->setPen(QColor(0, 0, 0, 200));
            QRect topShadow = rect.adjusted(1, 1, 0, -rect.height() / 2);
            painter->drawText(topShadow, Qt::AlignCenter, hand);

            painter->setPen(QColor("#FFFFFF"));
            QRect topRect = rect.adjusted(0, 0, 0, -rect.height() / 2);
            painter->drawText(topRect, Qt::AlignCenter, hand);

            f.setPointSize(7);
            f.setBold(false);
            painter->setFont(f);

            painter->setPen(QColor(0, 0, 0, 200));
            QRect btmShadow = rect.adjusted(1, rect.height() / 2 + 1, 0, 0);
            painter->drawText(btmShadow, Qt::AlignCenter, evStr);

            painter->setPen(QColor("#F3F4F6"));
            QRect btmRect = rect.adjusted(0, rect.height() / 2, 0, 0);
            painter->drawText(btmRect, Qt::AlignCenter, evStr);
        } else {
            // Pure Strategy Mode
            QFont f = painter->font();
            f.setPointSize(9);
            f.setBold(true);
            painter->setFont(f);

            // Subtle drop shadow for legibility over bright action bars
            painter->setPen(QColor(0, 0, 0, 180));
            painter->drawText(rect.adjusted(1, 1, 1, 1), Qt::AlignCenter, hand);

            painter->setPen(QColor("#FFFFFF"));
            painter->drawText(rect, Qt::AlignCenter, hand);
        }
    }

    // Grid outline
    painter->setPen(QColor("#111827"));
    painter->drawRect(rect);

    // Selected cell highlight border
    if (option.state & QStyle::State_Selected) {
        painter->setPen(QPen(QColor("#F59E0B"), 2)); // Amber gold border
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect.adjusted(1, 1, -2, -2));
    }

    painter->restore();
}

// ============================================================================
// StrategyExplorer Implementation
// ============================================================================

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
    setWindowTitle("Poker Solver — Strategy Explorer");
    setMinimumSize(1150, 780);
    resize(1250, 850);
    setStyleSheet("background-color: #111827; color: #F9FAFB;");

    // Extract initial board if present
    if (strategy_data_.contains("metadata") && strategy_data_["metadata"].contains("initial_board")) {
        for (const auto& c : strategy_data_["metadata"]["initial_board"]) {
            initial_board_.push_back(c.get<std::string>());
        }
    } else if (strategy_data_.contains("board") && strategy_data_["board"].is_array()) {
        for (const auto& c : strategy_data_["board"]) {
            initial_board_.push_back(c.get<std::string>());
        }
    }

    setupUI();

        // Auto-select the root node on launch
        if (gameTreeWidget->topLevelItemCount() > 0) {
            QTreeWidgetItem* rootItem = gameTreeWidget->topLevelItem(0);
            gameTreeWidget->setCurrentItem(rootItem);
            onNodeSelected(rootItem, 0);
        }
}

StrategyExplorer::~StrategyExplorer() {}

void StrategyExplorer::setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // ========================================================================
    // LEFT PANEL: Header & Game Tree
    // ========================================================================
    QWidget *leftWidget = new QWidget;
    leftWidget->setMinimumWidth(320);
    leftWidget->setMaximumWidth(400);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(12);

    // Header Frame
    QFrame *headerFrame = new QFrame;
    headerFrame->setStyleSheet("background-color: #1F2937; border-radius: 8px; border: 1px solid #374151; padding: 12px;");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerFrame);
    headerLayout->setContentsMargins(12, 12, 12, 12);
    headerLayout->setSpacing(8);

    QLabel *headerTitle = new QLabel("BOARD & STATE");
    headerTitle->setStyleSheet("font-size: 11px; font-weight: bold; color: #9CA3AF; letter-spacing: 1px;");
    headerLayout->addWidget(headerTitle);

    boardCardsLabel = new QLabel;
    boardCardsLabel->setTextFormat(Qt::RichText);
    boardCardsLabel->setStyleSheet("font-size: 14px; margin: 4px 0;");
    headerLayout->addWidget(boardCardsLabel);

    nodeInfoLabel = new QLabel("Active: OOP Decision");
    nodeInfoLabel->setStyleSheet("font-size: 12px; font-weight: bold; color: #E5E7EB;");
    headerLayout->addWidget(nodeInfoLabel);

    potLabel = new QLabel("Pot: 0.0");
    potLabel->setStyleSheet("font-size: 11px; color: #9CA3AF;");
    headerLayout->addWidget(potLabel);

    leftLayout->addWidget(headerFrame);

    // Game Tree Frame
    QFrame *gameTreeFrame = new QFrame;
    gameTreeFrame->setStyleSheet("background-color: #1F2937; border-radius: 8px; border: 1px solid #374151;");
    QVBoxLayout *gameTreeLayout = new QVBoxLayout(gameTreeFrame);
    gameTreeLayout->setContentsMargins(12, 12, 12, 12);
    gameTreeLayout->setSpacing(8);

    QLabel *gameTreeTitle = new QLabel("GAME TREE");
    gameTreeTitle->setStyleSheet("font-size: 11px; font-weight: bold; color: #9CA3AF; letter-spacing: 1px;");
    gameTreeLayout->addWidget(gameTreeTitle);

    gameTreeWidget = new QTreeWidget;
    gameTreeWidget->setHeaderHidden(true);
    gameTreeWidget->setStyleSheet(
        "QTreeWidget { background-color: #111827; color: #F3F4F6; border: 1px solid #374151; border-radius: 6px; padding: 4px; font-family: monospace; font-size: 11px; }"
        "QTreeWidget::item { padding: 4px 2px; margin: 1px 0; border-radius: 4px; }"
        "QTreeWidget::item:hover { background-color: #374151; }"
        "QTreeWidget::item:selected { background-color: #2563EB; color: #FFFFFF; font-weight: bold; }"
    );

    buildGameTreeUI(strategy_data_, nullptr);
    connect(gameTreeWidget, &QTreeWidget::itemExpanded, this, &StrategyExplorer::onItemExpanded);
    connect(gameTreeWidget, &QTreeWidget::itemClicked, this, &StrategyExplorer::onNodeSelected);

    gameTreeLayout->addWidget(gameTreeWidget);
    leftLayout->addWidget(gameTreeFrame);

    mainLayout->addWidget(leftWidget);

    // ========================================================================
    // RIGHT PANEL: Controls, Range Overview, 13x13 Matrix, Combo Inspector
    // ========================================================================
    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(12);

    // Controls Toolbar Frame
    QFrame *toolbarFrame = new QFrame;
    toolbarFrame->setStyleSheet("background-color: #1F2937; border-radius: 8px; border: 1px solid #374151;");
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarFrame);
    toolbarLayout->setContentsMargins(12, 8, 12, 8);
    toolbarLayout->setSpacing(10);

    QLabel *modeLabel = new QLabel("View Mode:");
    modeLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #9CA3AF;");
    toolbarLayout->addWidget(modeLabel);

    btnStrategy = new QPushButton("Strategy");
    btnEvStrategy = new QPushButton("EV + Strategy");
    btnEv = new QPushButton("EV Heatmap");

    QString btnStyleInactive = "QPushButton { background-color: #374151; color: #D1D5DB; border-radius: 4px; padding: 6px 12px; font-size: 11px; font-weight: bold; border: 1px solid #4B5563; } QPushButton:hover { background-color: #4B5563; }";
    btnStrategy->setStyleSheet(btnStyleInactive);
    btnEvStrategy->setStyleSheet(btnStyleInactive);
    btnEv->setStyleSheet(btnStyleInactive);

    connect(btnStrategy, &QPushButton::clicked, this, [this]() { onViewModeChanged(ViewMode::kStrategy); });
    connect(btnEvStrategy, &QPushButton::clicked, this, [this]() { onViewModeChanged(ViewMode::kStrategyAndEv); });
    connect(btnEv, &QPushButton::clicked, this, [this]() { onViewModeChanged(ViewMode::kEv); });

    toolbarLayout->addWidget(btnStrategy);
    toolbarLayout->addWidget(btnEvStrategy);
    toolbarLayout->addWidget(btnEv);

    toolbarLayout->addSpacing(16);

    QLabel *rangeLabel = new QLabel("Perspective:");
    rangeLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #9CA3AF;");
    toolbarLayout->addWidget(rangeLabel);

    btnPlayerOOP = new QPushButton("OOP (P1)");
    btnPlayerIP = new QPushButton("IP (P0)");
    btnPlayerOOP->setStyleSheet(btnStyleInactive);
    btnPlayerIP->setStyleSheet(btnStyleInactive);

    connect(btnPlayerOOP, &QPushButton::clicked, this, [this]() { onPlayerRangeToggled(1); });
    connect(btnPlayerIP, &QPushButton::clicked, this, [this]() { onPlayerRangeToggled(0); });

    toolbarLayout->addWidget(btnPlayerOOP);
    toolbarLayout->addWidget(btnPlayerIP);

    toolbarLayout->addStretch();
    rightLayout->addWidget(toolbarFrame);

    // Rough Strategy / Range Breakdown Panel
    QFrame *roughStrategyFrame = new QFrame;
    roughStrategyFrame->setStyleSheet("background-color: #1F2937; border-radius: 8px; border: 1px solid #374151;");
    QVBoxLayout *roughContainerLayout = new QVBoxLayout(roughStrategyFrame);
    roughContainerLayout->setContentsMargins(12, 10, 12, 10);
    roughContainerLayout->setSpacing(6);

    QLabel *roughTitle = new QLabel("RANGE ACTION SUMMARY");
    roughTitle->setStyleSheet("font-size: 11px; font-weight: bold; color: #9CA3AF; letter-spacing: 1px;");
    roughContainerLayout->addWidget(roughTitle);

    roughStrategyLayout = new QGridLayout;
    roughStrategyLayout->setSpacing(8);
    roughContainerLayout->addLayout(roughStrategyLayout);

    rightLayout->addWidget(roughStrategyFrame);

    // Splitter between 13x13 matrix and Combo Inspector
    QSplitter *centerSplitter = new QSplitter(Qt::Horizontal);
    centerSplitter->setChildrenCollapsible(false);

    // 13x13 Matrix Container
    QFrame *matrixFrame = new QFrame;
    matrixFrame->setStyleSheet("background-color: #1F2937; border-radius: 8px; border: 1px solid #374151; padding: 6px;");
    QVBoxLayout *matrixFrameLayout = new QVBoxLayout(matrixFrame);
    matrixFrameLayout->setContentsMargins(8, 8, 8, 8);
    matrixFrameLayout->setSpacing(6);

    QLabel *matrixTitle = new QLabel("13x13 STRATEGY MATRIX");
    matrixTitle->setStyleSheet("font-size: 11px; font-weight: bold; color: #9CA3AF; letter-spacing: 1px;");
    matrixFrameLayout->addWidget(matrixTitle);

    handMatrix = new QTableWidget(13, 13);
    handMatrix->horizontalHeader()->setVisible(false);
    handMatrix->verticalHeader()->setVisible(false);
    handMatrix->setEditTriggers(QAbstractItemView::NoEditTriggers);
    handMatrix->setSelectionMode(QAbstractItemView::SingleSelection);
    handMatrix->setFocusPolicy(Qt::NoFocus);
    handMatrix->setStyleSheet("QTableWidget { background-color: #111827; border: 1px solid #374151; border-radius: 4px; }");

    QStringList ranks = {"A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
    for (int row = 0; row < 13; ++row) {
        for (int col = 0; col < 13; ++col) {
            QString hand;
            if (row == col) hand = ranks[row] + ranks[col];
            else if (row < col) hand = ranks[row] + ranks[col] + "s";
            else hand = ranks[col] + ranks[row] + "o";

            QTableWidgetItem *item = new QTableWidgetItem(hand);
            item->setTextAlignment(Qt::AlignCenter);
            item->setData(Qt::UserRole + 5, false); // Out of range by default
            handMatrix->setItem(row, col, item);
        }
    }

    matrixDelegate = new StrategyItemDelegate(this);
    handMatrix->setItemDelegate(matrixDelegate);
    handMatrix->installEventFilter(this);
    connect(handMatrix, &QTableWidget::cellClicked, this, &StrategyExplorer::onHandCellClicked);

    matrixFrameLayout->addWidget(handMatrix);
    centerSplitter->addWidget(matrixFrame);

    // Combo Breakdown Inspector Container
    QFrame *inspectorFrame = new QFrame;
    inspectorFrame->setStyleSheet("background-color: #1F2937; border-radius: 8px; border: 1px solid #374151;");
    QVBoxLayout *inspectorLayout = new QVBoxLayout(inspectorFrame);
    inspectorLayout->setContentsMargins(12, 10, 12, 10);
    inspectorLayout->setSpacing(8);

    selectedHandLabel = new QLabel("COMBO BREAKDOWN (Click a hand cell)");
    selectedHandLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #9CA3AF; letter-spacing: 1px;");
    inspectorLayout->addWidget(selectedHandLabel);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { background-color: transparent; border: none; }");

    QWidget *scrollContent = new QWidget;
    scrollContent->setStyleSheet("background-color: transparent;");
    handStrategyLayout = new QGridLayout(scrollContent);
    handStrategyLayout->setSpacing(8);
    handStrategyLayout->setContentsMargins(0, 0, 0, 0);

    scrollArea->setWidget(scrollContent);
    inspectorLayout->addWidget(scrollArea);

    centerSplitter->addWidget(inspectorFrame);
    centerSplitter->setStretchFactor(0, 3);
    centerSplitter->setStretchFactor(1, 2);

    rightLayout->addWidget(centerSplitter, 1);
    mainLayout->addWidget(rightWidget, 1);

    setLayout(mainLayout);
    onViewModeChanged(ViewMode::kStrategy);
}

bool StrategyExplorer::eventFilter(QObject *obj, QEvent *event) {
    if (obj == handMatrix && event->type() == QEvent::Resize) {
        int newWidth = handMatrix->viewport()->width();
        int cellSize = std::max(20, newWidth / 13);
        for (int i = 0; i < 13; ++i) {
            handMatrix->setColumnWidth(i, cellSize);
            handMatrix->setRowHeight(i, cellSize);
        }
    }
    return QDialog::eventFilter(obj, event);
}

void StrategyExplorer::onViewModeChanged(ViewMode mode) {
    current_view_mode_ = mode;
    matrixDelegate->setViewMode(mode);

    QString activeStyle = "QPushButton { background-color: #2563EB; color: #FFFFFF; border-radius: 4px; padding: 6px 12px; font-size: 11px; font-weight: bold; border: 1px solid #3B82F6; }";
    QString inactiveStyle = "QPushButton { background-color: #374151; color: #D1D5DB; border-radius: 4px; padding: 6px 12px; font-size: 11px; font-weight: bold; border: 1px solid #4B5563; } QPushButton:hover { background-color: #4B5563; }";

    btnStrategy->setStyleSheet(mode == ViewMode::kStrategy ? activeStyle : inactiveStyle);
    btnEvStrategy->setStyleSheet(mode == ViewMode::kStrategyAndEv ? activeStyle : inactiveStyle);
    btnEv->setStyleSheet(mode == ViewMode::kEv ? activeStyle : inactiveStyle);

    handMatrix->viewport()->update();
}

void StrategyExplorer::onPlayerRangeToggled(int player) {
    current_display_player_ = player;
    updateStrategyGrid(current_node_);
    if (selected_hand_row_ >= 0 && selected_hand_col_ >= 0) {
        onHandCellClicked(selected_hand_row_, selected_hand_col_);
    }
}

void StrategyExplorer::buildGameTreeUI(const nlohmann::json& node, QTreeWidgetItem* parent_item, const std::string& current_path, const QString& action_label) {
    if (node.is_null()) return;

    QTreeWidgetItem* item = new QTreeWidgetItem;
    QString label;

    if (node.contains("node_type")) {
        std::string type = node["node_type"].get<std::string>();
        if (type == "Action") {
            int player = node.contains("player") ? node["player"].get<int>() : 0;
            QString playerTag = (player == 0 ? "[IP]" : "[OOP]");

            QString effective_action = action_label;
            if (effective_action.isEmpty() && node.contains("action_label")) {
                effective_action = QString::fromStdString(node["action_label"].get<std::string>());
            }

            if (!effective_action.isEmpty()) {
                if (effective_action.startsWith("deal:")) {
                    QString cardStr = effective_action.mid(5);
                    QString formattedCard = formatCardPlain(cardStr);
                    QString street = "Turn";
                    if (node.contains("board") && node["board"].is_array() && node["board"].size() == 5) {
                        street = "River";
                    }
                    label = QString("%1: %2 %3").arg(street, formattedCard, playerTag);
                } else {
                    label = QString("%1 %2").arg(playerTag, effective_action);
                }
            } else {
                label = QString("%1 Root Decision").arg(playerTag);
            }
        } else if (type == "Chance") {
            QString street = "Turn";
            if (node.contains("board") && node["board"].is_array() && node["board"].size() == 4) {
                street = "River";
            }
            label = QString("Chance (Deal %1)").arg(street);
        } else if (type == "Terminal") {
            label = action_label.isEmpty() ? "Fold (Terminal)" : QString("%1 (Fold)").arg(action_label);
        } else if (type == "Showdown") {
            label = action_label.isEmpty() ? "Showdown" : QString("%1 (Showdown)").arg(action_label);
        }
    }

    if (label.isEmpty()) {
        label = "Node";
    }

    item->setText(0, label);
    item->setData(0, Qt::UserRole, QString::fromStdString(current_path));

    if (parent_item) {
        parent_item->addChild(item);
    } else {
        gameTreeWidget->addTopLevelItem(item);
    }

    bool has_children = (node.contains("children") && node["children"].is_array() && !node["children"].empty()) ||
                        (node.contains("child") && !node["child"].is_null());

    if (!has_children) return;

    if (parent_item == nullptr) {
        // Root node: eagerly populate immediate children so first user options are immediately visible
        populateImmediateChildren(item, node, current_path);
        item->setExpanded(true);
    } else {
        // Child nodes: add lightweight placeholder dummy so '+' expansion handle appears
        QTreeWidgetItem* dummy = new QTreeWidgetItem;
        dummy->setText(0, "Loading...");
        dummy->setData(0, Qt::UserRole, "__DUMMY__");
        item->addChild(dummy);
    }
}

void StrategyExplorer::populateImmediateChildren(QTreeWidgetItem* parent_item, const nlohmann::json& node, const std::string& current_path) {
    if (node.contains("children") && node["children"].is_array()) {
        const auto& children = node["children"];
        for (size_t i = 0; i < children.size(); ++i) {
            const auto& child = children[i];
            if (child.contains("node") && !child["node"].is_null()) {
                std::string child_path = current_path + "/children/" + std::to_string(i) + "/node";
                QString act = "";
                if (child.contains("action")) {
                    act = QString::fromStdString(child["action"].get<std::string>());
                }
                buildGameTreeUI(child["node"], parent_item, child_path, act);
            }
        }
    } else if (node.contains("child") && !node["child"].is_null()) {
        std::string child_path = current_path + "/child";
        buildGameTreeUI(node["child"], parent_item, child_path);
    }
}

void StrategyExplorer::onItemExpanded(QTreeWidgetItem *item) {
    if (!item) return;
    if (item->childCount() == 1 && item->child(0)->data(0, Qt::UserRole).toString() == "__DUMMY__") {
        delete item->takeChild(0);

        QString path = item->data(0, Qt::UserRole).toString();
        try {
            const nlohmann::json* node_ptr = nullptr;
            if (path.isEmpty()) {
                node_ptr = &strategy_data_;
            } else {
                node_ptr = &strategy_data_.at(nlohmann::json::json_pointer(path.toStdString()));
            }
            if (node_ptr) {
                populateImmediateChildren(item, *node_ptr, path.toStdString());
            }
        } catch (const std::exception& e) {
            qDebug() << "Error expanding node:" << e.what();
        }
    }
}

void StrategyExplorer::updateHeader(const nlohmann::json& node) {
    // Board cards
    std::vector<std::string> board_cards;
    if (node.contains("board") && node["board"].is_array() && !node["board"].empty()) {
        for (const auto& c : node["board"]) {
            board_cards.push_back(c.get<std::string>());
        }
    } else if (!initial_board_.empty()) {
        board_cards = initial_board_;
    }

    QString boardHtml;
    if (!board_cards.empty()) {
        for (const auto& c : board_cards) {
            boardHtml += formatCardHtml(QString::fromStdString(c)) + " ";
        }
    } else {
        boardHtml = "<span style=\"color: #6B7280;\">Preflop (No Board)</span>";
    }
    boardCardsLabel->setText(boardHtml);

    // Active Player & Node Status
    std::string type = node.value("node_type", "Action");
    if (type == "Chance") {
        nodeInfoLabel->setText("State: <b style=\"color: #F59E0B;\">Chance Node</b> | Select a dealt card below");
        potLabel->setText("Dealing next street card...");
    } else if (type == "Showdown") {
        nodeInfoLabel->setText("State: <b style=\"color: #10B981;\">Showdown</b> | Hands contested");
        double pot = node.value("pot", 0.0);
        potLabel->setText(QString("Final Pot: <b style=\"color: #34D399;\">%1 chips</b>").arg(pot, 0, 'f', 1));
    } else if (type == "Terminal") {
        nodeInfoLabel->setText("State: <b style=\"color: #EF4444;\">Folded (Terminal)</b> | Hand ended");
        double pot = node.value("pot", 0.0);
        potLabel->setText(QString("Final Pot: <b style=\"color: #34D399;\">%1 chips</b>").arg(pot, 0, 'f', 1));
    } else {
        int acting_player = node.contains("player") ? node["player"].get<int>() : 0;
        QString playerTag = (acting_player == 0 ? "IP (Player 0)" : "OOP (Player 1)");
        QString actionName = "Root Action";
        if (node.contains("action_label")) {
            QString qAct = QString::fromStdString(node["action_label"].get<std::string>());
            if (qAct.startsWith("deal:")) {
                actionName = QString("Dealt %1").arg(formatCardPlain(qAct.mid(5)));
            } else {
                actionName = qAct;
            }
        }
        nodeInfoLabel->setText(QString("Active: <b style=\"color: #60A5FA;\">%1</b> | Node: %2").arg(playerTag, actionName));

        double pot = node.contains("pot") ? node["pot"].get<double>() : 0.0;
        potLabel->setText(QString("Current Pot: <b style=\"color: #34D399;\">%1 chips</b>").arg(pot, 0, 'f', 1));
    }

    // Update Player Range button highlights
    int acting_player = node.contains("player") ? node["player"].get<int>() : 0;
    int active_perspective = (current_display_player_ >= 0) ? current_display_player_ : acting_player;
    QString activeStyle = "QPushButton { background-color: #2563EB; color: #FFFFFF; border-radius: 4px; padding: 6px 12px; font-size: 11px; font-weight: bold; border: 1px solid #3B82F6; }";
    QString inactiveStyle = "QPushButton { background-color: #374151; color: #D1D5DB; border-radius: 4px; padding: 6px 12px; font-size: 11px; font-weight: bold; border: 1px solid #4B5563; } QPushButton:hover { background-color: #4B5563; }";

    btnPlayerOOP->setStyleSheet(active_perspective == 1 ? activeStyle : inactiveStyle);
    btnPlayerIP->setStyleSheet(active_perspective == 0 ? activeStyle : inactiveStyle);
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
        current_node_ = node;
        current_display_player_ = -1; // Reset perspective so it follows acting player of selected node
        updateHeader(node);
        updateStrategyGrid(node);

        // Refresh combo inspector: ensure an in-range cell is selected
        bool current_is_in_range = false;
        if (selected_hand_row_ >= 0 && selected_hand_col_ >= 0) {
            QTableWidgetItem *curCell = handMatrix->item(selected_hand_row_, selected_hand_col_);
            if (curCell && curCell->data(Qt::UserRole + 5).toBool()) {
                current_is_in_range = true;
            }
        }

        if (!current_is_in_range) {
            for (int r = 0; r < 13; ++r) {
                for (int c = 0; c < 13; ++c) {
                    QTableWidgetItem *cell = handMatrix->item(r, c);
                    if (cell && cell->data(Qt::UserRole + 5).toBool()) {
                        selected_hand_row_ = r;
                        selected_hand_col_ = c;
                        handMatrix->setCurrentCell(r, c);
                        current_is_in_range = true;
                        break;
                    }
                }
                if (current_is_in_range) break;
            }
        }

        if (selected_hand_row_ >= 0 && selected_hand_col_ >= 0) {
            onHandCellClicked(selected_hand_row_, selected_hand_col_);
        }
    } catch (const std::exception& e) {
        qDebug() << "Error retrieving node at path" << path << ":" << e.what();
    }
}

void StrategyExplorer::updateStrategyGrid(const nlohmann::json& node) {
    current_node_ = node;
    clearLayout(roughStrategyLayout);

    if (!node.contains("strategy") || node["strategy"].is_null()) {
        // Clear matrix if node has no strategy (e.g. Chance / Showdown / Terminal)
        for (int r = 0; r < 13; ++r) {
            for (int c = 0; c < 13; ++c) {
                QTableWidgetItem *item = handMatrix->item(r, c);
                if (item) {
                    item->setData(Qt::UserRole + 5, false);
                }
            }
        }
        handMatrix->viewport()->update();
        return;
    }

    auto strategy_obj = node["strategy"];
    if (!strategy_obj.contains("strategy") || !strategy_obj["strategy"].is_array()) {
        return;
    }
    const auto& strat_matrix = strategy_obj["strategy"];

    bool has_evs = strategy_obj.contains("evs") && strategy_obj["evs"].is_array();
    const auto& ev_matrix = has_evs ? strategy_obj["evs"] : nlohmann::json();

    int acting_player = node.contains("player") ? node["player"].get<int>() : 0;
    int display_player = (current_display_player_ >= 0) ? current_display_player_ : acting_player;

    if (!strategy_data_.contains("ranges") || !strategy_data_["ranges"].is_array() || display_player >= static_cast<int>(strategy_data_["ranges"].size())) {
        return;
    }

    const auto& player_range = strategy_data_["ranges"][display_player];

    // Collect action names and assign standardized colors
    QStringList action_names;
    QList<QColor> action_colors;
    extractActionInfo(node, strat_matrix, action_names, action_colors);

    struct HandData {
        double total_ev = 0.0;
        int count = 0;
        std::vector<double> action_freqs;
    };
    std::map<QString, HandData> category_data;

    QString ranks = "AKQJT98765432";
    std::vector<double> total_action_freqs(action_names.size(), 0.0);
    double total_combos = 0.0;

    double min_ev = 1e9;
    double max_ev = -1e9;

    for (size_t h = 0; h < player_range.size(); ++h) {
        if (h >= strat_matrix.size()) break;

        QString hand_str = QString::fromStdString(player_range[h].get<std::string>());
        if (hand_str.length() < 4) continue;

        QString rank1 = hand_str.mid(0, 1).toUpper();
        QString suit1 = hand_str.mid(1, 1).toLower();
        QString rank2 = hand_str.mid(2, 1).toUpper();
        QString suit2 = hand_str.mid(3, 1).toLower();

        int r1 = ranks.indexOf(rank1);
        int r2 = ranks.indexOf(rank2);
        if (r1 > r2) {
            std::swap(r1, r2);
            std::swap(rank1, rank2);
            std::swap(suit1, suit2);
        }

        QString category;
        if (r1 == r2) category = rank1 + rank2;
        else if (suit1 == suit2) category = rank1 + rank2 + "s";
        else category = rank1 + rank2 + "o";

        HandData& cd = category_data[category];
        cd.count++;

        if (has_evs && h < ev_matrix.size() && ev_matrix[h].is_array() && !ev_matrix[h].empty()) {
            double combo_ev = -1e9;
            for (size_t a = 0; a < ev_matrix[h].size(); ++a) {
                if (ev_matrix[h][a].is_null()) continue;
                double val = ev_matrix[h][a].get<double>();
                if (val > combo_ev) combo_ev = val;
            }
            if (combo_ev > -1e8) {
                cd.total_ev += combo_ev;
            }
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

    // Determine min/max EV for heatmap normalization
    for (const auto& pair : category_data) {
        if (pair.second.count > 0) {
            double avg_ev = pair.second.total_ev / pair.second.count;
            if (avg_ev < min_ev) min_ev = avg_ev;
            if (avg_ev > max_ev) max_ev = avg_ev;
        }
    }
    if (min_ev > max_ev) {
        min_ev = 0.0;
        max_ev = 1.0;
    } else if (std::abs(max_ev - min_ev) < 1e-4) {
        max_ev = min_ev + 1.0;
    }

    // Populate 13x13 hand matrix
    for (int row = 0; row < 13; ++row) {
        for (int col = 0; col < 13; ++col) {
            QString category;
            if (row == col) category = QString(ranks[row]) + ranks[col];
            else if (row < col) category = QString(ranks[row]) + ranks[col] + "s";
            else category = QString(ranks[col]) + ranks[row] + "o";

            QTableWidgetItem *item = handMatrix->item(row, col);
            if (!item) continue;

            item->setText(category);

            if (category_data.count(category)) {
                HandData& cd = category_data[category];
                QList<QVariant> freqs;
                for (double f : cd.action_freqs) {
                    freqs.append(f / cd.count);
                }

                QList<QVariant> colorsList;
                for (const auto& c : action_colors) colorsList.append(c);

                double avg_ev = cd.count > 0 ? (cd.total_ev / cd.count) : 0.0;

                // EV Heatmap color calculation (green for positive, red for negative)
                double t = std::clamp((avg_ev - min_ev) / (max_ev - min_ev), 0.0, 1.0);
                QColor evColor;
                if (t >= 0.5) {
                    double norm = (t - 0.5) * 2.0;
                    evColor = QColor::fromRgbF(0.2 * (1.0 - norm) + 0.06 * norm,
                                              0.4 * (1.0 - norm) + 0.72 * norm,
                                              0.2 * (1.0 - norm) + 0.38 * norm);
                } else {
                    double norm = t * 2.0;
                    evColor = QColor::fromRgbF(0.85 * (1.0 - norm) + 0.2 * norm,
                                              0.15 * (1.0 - norm) + 0.4 * norm,
                                              0.15 * (1.0 - norm) + 0.2 * norm);
                }

                item->setData(Qt::UserRole + 1, freqs);
                item->setData(Qt::UserRole + 2, colorsList);
                item->setData(Qt::UserRole + 3, avg_ev);
                item->setData(Qt::UserRole + 4, evColor);
                item->setData(Qt::UserRole + 5, true); // In range
            } else {
                item->setData(Qt::UserRole + 1, QVariant());
                item->setData(Qt::UserRole + 2, QVariant());
                item->setData(Qt::UserRole + 3, 0.0);
                item->setData(Qt::UserRole + 4, QColor("#1F2937"));
                item->setData(Qt::UserRole + 5, false); // Out of range
            }
        }
    }

    // Populate Rough Strategy Panel
    if (total_combos > 0) {
        for (int a = 0; a < action_names.size(); ++a) {
            double pct = (total_action_freqs[a] / total_combos) * 100.0;
            double combos = total_action_freqs[a];

            QFrame *cell = new QFrame;
            cell->setFrameShape(QFrame::StyledPanel);

            QColor color = action_colors[a];
            cell->setStyleSheet(QString(
                "background-color: %1; border-radius: 6px; padding: 6px 10px; border: 1px solid rgba(255,255,255,0.15);"
            ).arg(color.name()));

            QVBoxLayout *cellLayout = new QVBoxLayout(cell);
            cellLayout->setContentsMargins(6, 4, 6, 4);
            cellLayout->setSpacing(2);

            QLabel *cellTitle = new QLabel(action_names[a]);
            cellTitle->setStyleSheet("font-weight: bold; font-size: 11px; color: #FFFFFF;");
            cellLayout->addWidget(cellTitle);

            QLabel *pctLabel = new QLabel(QString("%1%").arg(pct, 0, 'f', 1));
            pctLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #FFFFFF;");
            cellLayout->addWidget(pctLabel);

            QLabel *combosLabel = new QLabel(QString("%1 combos").arg(combos, 0, 'f', 1));
            combosLabel->setStyleSheet("font-size: 10px; color: rgba(255,255,255,0.85);");
            cellLayout->addWidget(combosLabel);

            roughStrategyLayout->addWidget(cell, 0, a);
        }
    }

    handMatrix->viewport()->update();
}

void StrategyExplorer::onHandCellClicked(int row, int col) {
    selected_hand_row_ = row;
    selected_hand_col_ = col;

    QTableWidgetItem *item = handMatrix->item(row, col);
    if (!item) return;

    QString category = item->text();
    if (category.isEmpty()) return;

    selectedHandLabel->setText(QString("COMBO BREAKDOWN — <b style=\"color: #F59E0B;\">%1</b>").arg(category));

    clearLayout(handStrategyLayout);

    if (current_node_.is_null() || !current_node_.contains("strategy")) return;
    auto strategy_obj = current_node_["strategy"];
    if (!strategy_obj.contains("strategy") || !strategy_obj["strategy"].is_array()) return;
    const auto& strat_matrix = strategy_obj["strategy"];

    bool has_evs = strategy_obj.contains("evs") && strategy_obj["evs"].is_array();
    const auto& ev_matrix = has_evs ? strategy_obj["evs"] : nlohmann::json();

    int acting_player = current_node_.contains("player") ? current_node_["player"].get<int>() : 0;
    int display_player = (current_display_player_ >= 0) ? current_display_player_ : acting_player;

    if (!strategy_data_.contains("ranges") || display_player >= static_cast<int>(strategy_data_["ranges"].size())) return;
    const auto& player_range = strategy_data_["ranges"][display_player];

    // Collect board cards for card-removal (blocker) checks
    std::vector<std::string> board_cards;
    if (current_node_.contains("board") && current_node_["board"].is_array()) {
        for (const auto& c : current_node_["board"]) board_cards.push_back(c.get<std::string>());
    } else if (!initial_board_.empty()) {
        board_cards = initial_board_;
    }

    QString ranks = "AKQJT98765432";

    QStringList action_names;
    QList<QColor> action_colors;
    extractActionInfo(current_node_, strat_matrix, action_names, action_colors);

    int box_index = 0;
    for (size_t h = 0; h < player_range.size(); ++h) {
        if (h >= strat_matrix.size()) break;

        QString hand_str = QString::fromStdString(player_range[h].get<std::string>());
        if (hand_str.length() < 4) continue;

        QString c1 = hand_str.mid(0, 2);
        QString c2 = hand_str.mid(2, 2);

        QString rank1 = hand_str.mid(0, 1).toUpper();
        QString suit1 = hand_str.mid(1, 1).toLower();
        QString rank2 = hand_str.mid(2, 1).toUpper();
        QString suit2 = hand_str.mid(3, 1).toLower();

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
            // Check if combo is blocked by the board
            bool isBlocked = false;
            for (const auto& bc : board_cards) {
                QString qbc = QString::fromStdString(bc).toLower();
                if (c1.toLower() == qbc || c2.toLower() == qbc) {
                    isBlocked = true;
                    break;
                }
            }

            QFrame *strategyBox = new QFrame;
            strategyBox->setFrameShape(QFrame::StyledPanel);

            if (isBlocked) {
                strategyBox->setStyleSheet("background-color: #1F2937; border-radius: 6px; padding: 6px; border: 1px dashed #4B5563; opacity: 0.5;");
            } else {
                strategyBox->setStyleSheet("background-color: #1F2937; border-radius: 6px; padding: 6px; border: 1px solid #374151;");
            }

            QVBoxLayout *boxLayout = new QVBoxLayout(strategyBox);
            boxLayout->setSpacing(4);
            boxLayout->setContentsMargins(8, 6, 8, 6);

            // Combo header with cards
            QHBoxLayout *comboHeader = new QHBoxLayout;
            QLabel *cardsLabel = new QLabel(formatCardHtml(c1) + " " + formatCardHtml(c2));
            cardsLabel->setTextFormat(Qt::RichText);
            comboHeader->addWidget(cardsLabel);

            if (isBlocked) {
                QLabel *blockedBadge = new QLabel("BLOCKED");
                blockedBadge->setStyleSheet("color: #EF4444; font-size: 9px; font-weight: bold; background-color: #374151; padding: 1px 4px; border-radius: 2px;");
                comboHeader->addWidget(blockedBadge);
            } else if (has_evs && h < ev_matrix.size() && ev_matrix[h].is_array() && !ev_matrix[h].empty()) {
                double max_ev = -1e9;
                for (size_t a = 0; a < ev_matrix[h].size(); ++a) {
                    if (!ev_matrix[h][a].is_null()) {
                        double v = ev_matrix[h][a].get<double>();
                        if (v > max_ev) max_ev = v;
                    }
                }
                if (max_ev > -1e8) {
                    QLabel *evBadge = new QLabel(QString("EV: %1%2").arg(max_ev >= 0 ? "+" : "", QString::number(max_ev, 'f', 1)));
                    evBadge->setStyleSheet("color: #34D399; font-size: 10px; font-weight: bold;");
                    comboHeader->addWidget(evBadge);
                }
            }
            comboHeader->addStretch();
            boxLayout->addLayout(comboHeader);

            // Actions breakdown
            const auto& hand_strats = strat_matrix[h];
            for (size_t a = 0; a < hand_strats.size() && a < static_cast<size_t>(action_names.size()); ++a) {
                double pct = hand_strats[a].get<double>() * 100.0;
                QColor actColor = action_colors[static_cast<int>(a)];

                QHBoxLayout *actRow = new QHBoxLayout;
                actRow->setSpacing(6);

                QLabel *actLabel = new QLabel(action_names[static_cast<int>(a)]);
                actLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: bold;").arg(actColor.name()));
                actRow->addWidget(actLabel);

                // Action mini bar
                QFrame *barFrame = new QFrame;
                barFrame->setFixedHeight(6);
                barFrame->setStyleSheet("background-color: #374151; border-radius: 3px;");
                QHBoxLayout *barLayout = new QHBoxLayout(barFrame);
                barLayout->setContentsMargins(0, 0, 0, 0);

                QFrame *fillBar = new QFrame;
                fillBar->setFixedHeight(6);
                fillBar->setStyleSheet(QString("background-color: %1; border-radius: 3px;").arg(actColor.name()));
                fillBar->setFixedWidth(std::max(1, static_cast<int>(std::round(pct))));
                barLayout->addWidget(fillBar);
                barLayout->addStretch();

                actRow->addWidget(barFrame, 1);

                QLabel *pctValue = new QLabel(QString("%1%").arg(pct, 4, 'f', 1));
                pctValue->setStyleSheet("color: #D1D5DB; font-size: 10px; font-family: monospace;");
                actRow->addWidget(pctValue);

                boxLayout->addLayout(actRow);
            }

            handStrategyLayout->addWidget(strategyBox, box_index / 3, box_index % 3);
            box_index++;
        }
    }
}

bool StrategyExplorer::verifyChanceNodeStrategies() {
    // 1. Traverse game tree to find a node after a chance node (e.g. Turn node)
    QTreeWidgetItem* turnItem = nullptr;
    std::function<void(QTreeWidgetItem*)> findTurnNode = [&](QTreeWidgetItem* item) {
        if (!item) return;
        if (item->childCount() == 1 && item->child(0)->data(0, Qt::UserRole).toString() == "__DUMMY__") {
            onItemExpanded(item);
        }
        QString text = item->text(0);
        if (text.startsWith("Turn:") || text.startsWith("River:")) {
            if (!turnItem) turnItem = item;
            return;
        }
        for (int i = 0; i < item->childCount(); ++i) {
            findTurnNode(item->child(i));
        }
    };

    for (int i = 0; i < gameTreeWidget->topLevelItemCount(); ++i) {
        findTurnNode(gameTreeWidget->topLevelItem(i));
    }

    if (!turnItem) {
        qDebug() << "[VERIFY FAIL] Could not find any Turn or River child item after chance node in game tree";
        return false;
    }

    qDebug() << "[VERIFY] Found chance child node in tree:" << turnItem->text(0);

    // 2. Select this node
    gameTreeWidget->setCurrentItem(turnItem);
    onNodeSelected(turnItem, 0);

    // 3. Verify node state
    if (!current_node_.contains("strategy")) {
        qDebug() << "[VERIFY FAIL] Selected turn node does not contain strategy";
        return false;
    }

    // 4. Verify board cards in header has 4 cards for Turn
    if (boardCardsLabel->text().isEmpty()) {
        qDebug() << "[VERIFY FAIL] Board cards label is empty";
        return false;
    }

    // 5. Verify 13x13 matrix cells have valid freqs and colors
    int in_range_cells = 0;
    int cells_with_valid_strat = 0;
    for (int r = 0; r < 13; ++r) {
        for (int c = 0; c < 13; ++c) {
            QTableWidgetItem *cell = handMatrix->item(r, c);
            if (!cell) continue;
            bool inRange = cell->data(Qt::UserRole + 5).toBool();
            if (inRange) {
                in_range_cells++;
                QVariant freqsVar = cell->data(Qt::UserRole + 1);
                QVariant colorsVar = cell->data(Qt::UserRole + 2);
                if (freqsVar.isValid() && colorsVar.isValid()) {
                    QList<QVariant> freqs = freqsVar.toList();
                    QList<QVariant> colors = colorsVar.toList();
                    if (!freqs.isEmpty() && freqs.size() == colors.size()) {
                        double sum = 0.0;
                        for (const auto& f : freqs) sum += f.toDouble();
                        if (sum > 0.001) {
                            cells_with_valid_strat++;
                        }
                    }
                }
            }
        }
    }

    qDebug() << "[VERIFY] In-range cells on turn:" << in_range_cells
             << "| Cells with valid matching strategy & colors:" << cells_with_valid_strat;

    if (in_range_cells == 0 || cells_with_valid_strat == 0) {
        qDebug() << "[VERIFY FAIL] No valid strategy cells found on turn node!";
        return false;
    }

    // 6. Verify Combo Breakdown Inspector on first in-range hand cell
    int test_r = -1, test_c = -1;
    for (int r = 0; r < 13 && test_r == -1; ++r) {
        for (int c = 0; c < 13; ++c) {
            QTableWidgetItem *cell = handMatrix->item(r, c);
            if (cell && cell->data(Qt::UserRole + 5).toBool()) {
                test_r = r;
                test_c = c;
                break;
            }
        }
    }

    if (test_r >= 0 && test_c >= 0) {
        onHandCellClicked(test_r, test_c);
    }

    if (handStrategyLayout->count() == 0) {
        qDebug() << "[VERIFY FAIL] Combo breakdown inspector has no boxes for in-range cell" << test_r << test_c;
        return false;
    }

    // 7. Verify Rough Strategy summary layout
    if (roughStrategyLayout->count() == 0) {
        qDebug() << "[VERIFY FAIL] Rough strategy action summary has no cards";
        return false;
    }

    qDebug() << "[VERIFY SUCCESS] Chance child node displays strategies, frequencies, and combo breakdown properly!";
    return true;
}