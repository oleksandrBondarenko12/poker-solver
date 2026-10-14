#include "solver_main_window.h"
#include "range_matrix_dialog.h"
#include "board_card_dialog.h"
#include "gto_strategy_explorer.h"
#include <QApplication>
#include <QStyle>
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QSpacerItem>
#include <QScrollArea>
#include <QSizePolicy>
#include <QTextEdit>
#include <fstream>
#include "poker_solver/tree/scenario_game_rule.h"
#include "poker_solver/ranges/range_syntax_parser.h"
#include "poker_solver/ranges/range_distribution_manager.h"
#include "poker_solver/ranges/river_evaluation_cache.h"
#include "poker_solver/evaluator/table_hand_evaluator.h"
#include "poker_solver/tree/extensive_game_tree.h"
#include "poker_solver/core/poker_card.h"
#include "poker_solver/solver/public_chance_cfr_solver.h"
#include "../../tests/scenario_json_loader.h"

// Define inputStyle as a static const member variable
const QString MainWindow::inputStyle = 
    "QLineEdit {"
    "  background-color: #1F2937;"
    "  color: #F9FAFB;"
    "  padding: 8px 12px;"
    "  border-radius: 6px;"
    "  border: 1px solid #4B5563;"
    "}"
    "QLineEdit:focus {"
    "  border: 2px solid #3B82F6;"
    "}";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ipRange_ = "AA,KK,QQ,JJ,TT,99,88,AKs,AQs,AJs,ATs,KQs,KJs,AKo,AQo";
    oopRange_ = "AA,KK,QQ,JJ,TT,99,88,77,66,AKs,AQs,KQs,QJs,JTs,T9s,AKo,AQo";
    setupUI();
    applyStyles();
}

void MainWindow::setupUI() {
    // Create a scroll area to contain everything
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget *centralWidget = new QWidget(scrollArea);
    scrollArea->setWidget(centralWidget);
    setCentralWidget(scrollArea);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Header - make it stretch horizontally
    QWidget *headerContainer = new QWidget;
    headerContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    headerContainer->setStyleSheet("background-color: #1F2937; border-bottom: 1px solid #374151;");
    QVBoxLayout *headerLayout = new QVBoxLayout(headerContainer);
    headerLayout->setContentsMargins(16, 16, 16, 16);
    
    QLabel *header = new QLabel("Solver Strategy Parameters");
    header->setStyleSheet("font-size: 18px; font-weight: 600; color: #F9FAFB;");
    headerLayout->addWidget(header);
    mainLayout->addWidget(headerContainer);

    // Range Selection Section
    QLabel *rangeLabel = new QLabel("RANGE SELECTION");
    rangeLabel->setStyleSheet("color: #6B7280; font-size: 12px; font-weight: 500; margin-top: 16px;");
    mainLayout->addWidget(rangeLabel);

    QHBoxLayout *rangeLayout = new QHBoxLayout();
    rangeLayout->setSpacing(12);
    selectIPButton = new QPushButton(QString("IP Range (%1 combos)").arg(ipRange_.split(',', Qt::SkipEmptyParts).size()));
    selectOOPButton = new QPushButton(QString("OOP Range (%1 combos)").arg(oopRange_.split(',', Qt::SkipEmptyParts).size()));
    selectIPButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    selectOOPButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QString buttonStyle = "QPushButton {"
                         "  background-color: #1F2937;"
                         "  color: #F9FAFB;"
                         "  padding: 8px 16px;"
                         "  border-radius: 6px;"
                         "  font-weight: 500;"
                         "}"
                         "QPushButton:hover {"
                         "  background-color: #374151;"
                         "}";
    selectIPButton->setStyleSheet(buttonStyle);
    selectOOPButton->setStyleSheet(buttonStyle);
    rangeLayout->addWidget(selectIPButton);
    rangeLayout->addWidget(selectOOPButton);
    rangeLayout->addStretch();
    mainLayout->addLayout(rangeLayout);

    // Board Input Section
    QLabel *boardLabel = new QLabel("Board");
    boardLabel->setStyleSheet("color: #6B7280; font-size: 12px; font-weight: 500; margin-top: 12px;");
    mainLayout->addWidget(boardLabel);

    QHBoxLayout *boardLayout = new QHBoxLayout();
    boardLayout->setSpacing(12);
    boardInput = new QLineEdit("5d 9h 4s");
    boardInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    selectBoardButton = new QPushButton("Select Board Card");
    selectBoardButton->setStyleSheet(buttonStyle);
    boardLayout->addWidget(boardInput);
    boardLayout->addWidget(selectBoardButton);
    mainLayout->addLayout(boardLayout);

    // Street Controls Grid
    QGridLayout *streetLayout = new QGridLayout();
    streetLayout->setSpacing(16);
    for(int i = 0; i < streetLayout->columnCount(); ++i) {
        streetLayout->setColumnStretch(i, 1);
    }
    streetLayout->addWidget(createStreetControls("Flop IP", flopIP), 0, 0);
    streetLayout->addWidget(createStreetControls("Turn IP", turnIP), 0, 1);
    streetLayout->addWidget(createStreetControls("River IP", riverIP), 0, 2);
    mainLayout->addLayout(streetLayout);

    // Copy Button
    QPushButton *copyButton = new QPushButton("Copy from IP to OOP");
    copyButton->setStyleSheet("background-color: #1F2937; color: #ffffff; padding: 8px; border-radius: 4px;");
    mainLayout->addWidget(copyButton);

    // OOP Controls
    QGridLayout *oopLayout = new QGridLayout();
    for(int i = 0; i < oopLayout->columnCount(); ++i) {
        oopLayout->setColumnStretch(i, 1);
    }
    oopLayout->addWidget(createStreetControls("Flop OOP", flopOOP), 0, 0);
    oopLayout->addWidget(createStreetControls("Turn OOP", turnOOP), 0, 1);
    oopLayout->addWidget(createStreetControls("River OOP", riverOOP), 0, 2);
    mainLayout->addLayout(oopLayout);

    // Solver Configuration
    QGridLayout *configLayout = new QGridLayout();
    configLayout->setColumnStretch(1, 1); // Make input columns stretch
    configLayout->setColumnStretch(4, 1);
    
    // First row
    QLabel *raiseLimitLabel = new QLabel("raise limit:");
    raiseLimitInput = new QLineEdit("3");
    QLabel *timesLabel = new QLabel("times");
    configLayout->addWidget(raiseLimitLabel, 0, 0);
    raiseLimitInput->setStyleSheet(inputStyle);
    configLayout->addWidget(raiseLimitInput, 0, 1);
    configLayout->addWidget(timesLabel, 0, 2);

    QLabel *potLabel = new QLabel("Pot:");
    potInput = new QLineEdit("50");
    potInput->setStyleSheet(inputStyle);
    configLayout->addWidget(potLabel, 0, 3);
    configLayout->addWidget(potInput, 0, 4);

    QLabel *stackLabel = new QLabel("Effective Stack:");
    effectiveStackInput = new QLineEdit("200");
    effectiveStackInput->setStyleSheet(inputStyle);
    configLayout->addWidget(stackLabel, 0, 5);
    configLayout->addWidget(effectiveStackInput, 0, 6);

    // Second row
    QLabel *allinLabel = new QLabel("allin threshold:");
    allinThresholdInput = new QLineEdit("0.67");
    allinThresholdInput->setStyleSheet(inputStyle);
    configLayout->addWidget(allinLabel, 1, 0);
    configLayout->addWidget(allinThresholdInput, 1, 1);

    useIsomorphismCheck = new QCheckBox("use Isomorphism");
    configLayout->addWidget(useIsomorphismCheck, 1, 5);

    mainLayout->addLayout(configLayout);

    // Action Buttons
    buildTreeButton = new QPushButton("Build Tree");
    estimateMemoryButton = new QPushButton("Estimate Solving Memory");
    buildTreeButton->setStyleSheet("background-color: #1F2937; color: #ffffff; padding: 8px; border-radius: 4px;");
    estimateMemoryButton->setStyleSheet("background-color: #1F2937; color: #ffffff; padding: 8px; border-radius: 4px;");
    mainLayout->addWidget(buildTreeButton);
    mainLayout->addWidget(estimateMemoryButton);

    // Solver Parameters
    QGridLayout *solverLayout = new QGridLayout();
    
    QLabel *iterationsLabel = new QLabel("Iterations:");
    iterationsInput = new QLineEdit("200");
    iterationsInput->setStyleSheet(inputStyle);
    QLabel *timesLabel2 = new QLabel("times");
    solverLayout->addWidget(iterationsLabel, 0, 0);
    solverLayout->addWidget(iterationsInput, 0, 1);
    solverLayout->addWidget(timesLabel2, 0, 2);

    QLabel *exploitabilityLabel = new QLabel("stop solving when reach");
    exploitabilityInput = new QLineEdit("0.5");
    exploitabilityInput->setStyleSheet(inputStyle);
    QLabel *percentLabel = new QLabel("% exploitability");
    solverLayout->addWidget(exploitabilityLabel, 0, 3);
    solverLayout->addWidget(exploitabilityInput, 0, 4);
    solverLayout->addWidget(percentLabel, 0, 5);

    QLabel *logLabel = new QLabel("log interval:");
    logIntervalInput = new QLineEdit("10");
    logIntervalInput->setStyleSheet(inputStyle);
    QLabel *threadsLabel = new QLabel("threads:");
    threadsInput = new QLineEdit("8");
    threadsInput->setStyleSheet(inputStyle);
    solverLayout->addWidget(logLabel, 1, 0);
    solverLayout->addWidget(logIntervalInput, 1, 1);
    solverLayout->addWidget(threadsLabel, 1, 3);
    solverLayout->addWidget(threadsInput, 1, 4);

    mainLayout->addLayout(solverLayout);

    // Final Action Buttons
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(12);
    startSolvingButton = new QPushButton("Start Solving");
    stopSolvingButton = new QPushButton("Stop Solving");
    showResultButton = new QPushButton("Show Result");
    loadResultButton = new QPushButton("Load Result");
    
    startSolvingButton->setStyleSheet("QPushButton {"
                                     "  background-color: #3B82F6;"
                                     "  color: #FFFFFF;"
                                     "  padding: 8px 16px;"
                                     "  border-radius: 6px;"
                                     "  font-weight: 500;"
                                     "}"
                                     "QPushButton:hover {"
                                     "  background-color: #2563EB;"
                                     "}");
    stopSolvingButton->setStyleSheet("background-color: #1F2937; color: #ffffff; padding: 8px 16px; border-radius: 4px;");
    showResultButton->setStyleSheet("background-color: #1F2937; color: #ffffff; padding: 8px 16px; border-radius: 4px;");
    loadResultButton->setStyleSheet("background-color: #1F2937; color: #ffffff; padding: 8px 16px; border-radius: 4px;");
    
    startSolvingButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    stopSolvingButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    showResultButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    loadResultButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    actionLayout->addWidget(startSolvingButton);
    actionLayout->addWidget(stopSolvingButton);
    actionLayout->addWidget(showResultButton);
    actionLayout->addWidget(loadResultButton);
    mainLayout->addLayout(actionLayout);

    // Log Area
    logTextEdit = new QTextEdit(this);
    logTextEdit->setReadOnly(true);  // Make it read-only
    logTextEdit->setStyleSheet(
        "QTextEdit {"
        "  background-color: #1F2937;"
        "  color: #9CA3AF;"
        "  border: 1px solid #374151;"
        "  border-radius: 6px;"
        "  padding: 8px;"
        "  font-family: monospace;"
        "}"
    );
    logTextEdit->setMinimumHeight(200);  // Set a minimum height for the log area
    mainLayout->addWidget(logTextEdit);

    // Clear Log Button (moved after log area)
    clearLogButton = new QPushButton("Clear Log");
    clearLogButton->setStyleSheet(
        "QPushButton {"
        "  background-color: #1F2937;"
        "  color: #F9FAFB;"
        "  padding: 8px;"
        "  border-radius: 4px;"
        "  width: 100%;"
        "}"
        "QPushButton:hover {"
        "  background-color: #374151;"
        "}"
    );
    mainLayout->addWidget(clearLogButton);

    // Set minimum size for the window
    setMinimumSize(800, 600);

    // Connect signals
    connect(selectBoardButton, &QPushButton::clicked, this, &MainWindow::showBoardSelector);
    connect(selectIPButton, &QPushButton::clicked, this, &MainWindow::showIPRangeSelector);
    connect(selectOOPButton, &QPushButton::clicked, this, &MainWindow::showOOPRangeSelector);
    connect(copyButton, &QPushButton::clicked, this, &MainWindow::copyIPtoOOP);
    connect(buildTreeButton, &QPushButton::clicked, this, &MainWindow::buildTree);
    connect(estimateMemoryButton, &QPushButton::clicked, this, &MainWindow::estimateMemory);
    connect(startSolvingButton, &QPushButton::clicked, this, &MainWindow::startSolving);
    connect(stopSolvingButton, &QPushButton::clicked, this, &MainWindow::stopSolving);
    connect(showResultButton, &QPushButton::clicked, this, &MainWindow::showResult);
    connect(loadResultButton, &QPushButton::clicked, this, &MainWindow::loadResult);
    connect(clearLogButton, &QPushButton::clicked, this, &MainWindow::clearLog);
}

QWidget* MainWindow::createStreetControls(const QString& title, StreetControls& controls) {
    QWidget *widget = new QWidget;
    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setSpacing(12);
    
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("color: #6B7280; font-size: 12px; font-weight: 500;");
    
    QLabel *betLabel = new QLabel("Bet Sizes:");
    betLabel->setStyleSheet("color: #9CA3AF;");
    controls.betSizes = new QLineEdit("50");
    controls.betSizes->setStyleSheet(inputStyle);
    controls.betSizes->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    QLabel *raiseLabel = new QLabel("Raise Sizes:");
    raiseLabel->setStyleSheet("color: #9CA3AF;");
    controls.raiseSizes = new QLineEdit("60");
    controls.raiseSizes->setStyleSheet(inputStyle);
    controls.raiseSizes->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    controls.addAllIn = new QCheckBox("Add AllIn");
    controls.addAllIn->setStyleSheet("color: #9CA3AF;");
    controls.addAllIn->setChecked(true);
    
    layout->addWidget(titleLabel);
    layout->addWidget(betLabel);
    layout->addWidget(controls.betSizes);
    layout->addWidget(raiseLabel);
    layout->addWidget(controls.raiseSizes);
    layout->addWidget(controls.addAllIn);
    
    return widget;
}

void MainWindow::applyStyles() {
    // Load and apply stylesheet from resources
    QFile styleFile(":/styles/dark.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = styleFile.readAll();
        setStyleSheet(style);
    } else {
        qDebug() << "Failed to open stylesheet";
    }
}

void MainWindow::showBoardSelector() {
    BoardSelector selector(this, boardInput->text());
    if (selector.exec() == QDialog::Accepted) {
        boardInput->setText(selector.getSelectedBoard());
    }
}

poker_solver::config::Rule MainWindow::buildRuleFromUI(const std::vector<int>& initial_board_ints) {
    poker_solver::core::Deck deck;

    double pot = potInput->text().trimmed().toDouble();
    if (pot <= 0.0) pot = 50.0;
    double initial_oop_commit = pot / 2.0;
    double initial_ip_commit = pot / 2.0;

    double effective_stack = effectiveStackInput->text().trimmed().toDouble();
    if (effective_stack <= 0.0) effective_stack = 200.0;

    int raise_limit = raiseLimitInput->text().trimmed().toInt();
    if (raise_limit <= 0) raise_limit = 3;

    double all_in_threshold = allinThresholdInput->text().trimmed().toDouble();
    if (all_in_threshold <= 0.0) all_in_threshold = 0.67;

    poker_solver::core::GameRound starting_round = poker_solver::core::GameRound::kFlop;
    if (initial_board_ints.empty()) {
        starting_round = poker_solver::core::GameRound::kPreflop;
    } else if (initial_board_ints.size() == 3) {
        starting_round = poker_solver::core::GameRound::kFlop;
    } else if (initial_board_ints.size() == 4) {
        starting_round = poker_solver::core::GameRound::kTurn;
    } else if (initial_board_ints.size() == 5) {
        starting_round = poker_solver::core::GameRound::kRiver;
    }

    auto parseSizes = [](QLineEdit* edit) -> std::vector<double> {
        std::vector<double> sizes;
        if (!edit) return sizes;
        QString text = edit->text().trimmed();
        QStringList tokens = text.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
        for (const QString& tok : tokens) {
            bool ok = false;
            double val = tok.toDouble(&ok);
            if (ok && val > 0.0) {
                sizes.push_back(val);
            }
        }
        return sizes;
    };

    poker_solver::config::StreetSetting flop_ip(parseSizes(flopIP.betSizes), parseSizes(flopIP.raiseSizes), {}, flopIP.addAllIn->isChecked());
    poker_solver::config::StreetSetting flop_oop(parseSizes(flopOOP.betSizes), parseSizes(flopOOP.raiseSizes), {}, flopOOP.addAllIn->isChecked());
    poker_solver::config::StreetSetting turn_ip(parseSizes(turnIP.betSizes), parseSizes(turnIP.raiseSizes), {}, turnIP.addAllIn->isChecked());
    poker_solver::config::StreetSetting turn_oop(parseSizes(turnOOP.betSizes), parseSizes(turnOOP.raiseSizes), {}, turnOOP.addAllIn->isChecked());
    poker_solver::config::StreetSetting river_ip(parseSizes(riverIP.betSizes), parseSizes(riverIP.raiseSizes), {}, riverIP.addAllIn->isChecked());
    poker_solver::config::StreetSetting river_oop(parseSizes(riverOOP.betSizes), parseSizes(riverOOP.raiseSizes), {}, riverOOP.addAllIn->isChecked());

    poker_solver::config::GameTreeBuildingSettings settings(
        flop_ip, turn_ip, river_ip,
        flop_oop, turn_oop, river_oop
    );

    return poker_solver::config::Rule(
        deck,
        initial_oop_commit,
        initial_ip_commit,
        starting_round,
        initial_board_ints,
        raise_limit,
        1.0, 2.0, // small blind, big blind
        effective_stack,
        settings,
        all_in_threshold
    );
}

void MainWindow::showIPRangeSelector() {
    RangeSelector selector(this, true);
    selector.setSelectedRange(ipRange_);
    if (selector.exec() == QDialog::Accepted) {
        ipRange_ = selector.getSelectedRange();
        int count = ipRange_.split(',', Qt::SkipEmptyParts).size();
        selectIPButton->setText(QString("IP Range (%1 combos)").arg(count));
        appendToLog(QString("Updated IP range (%1 combos): %2").arg(count).arg(ipRange_));
    }
}

void MainWindow::showOOPRangeSelector() {
    RangeSelector selector(this, false);
    selector.setSelectedRange(oopRange_);
    if (selector.exec() == QDialog::Accepted) {
        oopRange_ = selector.getSelectedRange();
        int count = oopRange_.split(',', Qt::SkipEmptyParts).size();
        selectOOPButton->setText(QString("OOP Range (%1 combos)").arg(count));
        appendToLog(QString("Updated OOP range (%1 combos): %2").arg(count).arg(oopRange_));
    }
}

void MainWindow::copyIPtoOOP() {
    // Copy IP street control values to OOP controls
    flopOOP.betSizes->setText(flopIP.betSizes->text());
    flopOOP.raiseSizes->setText(flopIP.raiseSizes->text());
    flopOOP.addAllIn->setChecked(flopIP.addAllIn->isChecked());
    
    turnOOP.betSizes->setText(turnIP.betSizes->text());
    turnOOP.raiseSizes->setText(turnIP.raiseSizes->text());
    turnOOP.addAllIn->setChecked(turnIP.addAllIn->isChecked());
    
    riverOOP.betSizes->setText(riverIP.betSizes->text());
    riverOOP.raiseSizes->setText(riverIP.raiseSizes->text());
    riverOOP.addAllIn->setChecked(riverIP.addAllIn->isChecked());

    oopRange_ = ipRange_;
    int count = oopRange_.split(',', Qt::SkipEmptyParts).size();
    selectOOPButton->setText(QString("OOP Range (%1 combos)").arg(count));
    appendToLog(QString("Copied IP parameters and range (%1 combos) to OOP.").arg(count));
}

void MainWindow::buildTree() {
    appendToLog("Building game tree from user configuration...");
    try {
        QString boardText = boardInput->text().trimmed();
        QStringList cardTokens = boardText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        std::vector<int> initial_board_ints;
        for (const QString& tok : cardTokens) {
            auto val = poker_solver::core::Card::StringToInt(tok.toStdString());
            if (val) initial_board_ints.push_back(*val);
        }

        auto rule = buildRuleFromUI(initial_board_ints);
        poker_solver::tree::GameTree tree(rule);

        appendToLog(QString("Game tree built successfully: %1 action nodes, %2 chance nodes, %3 terminal nodes, %4 showdown nodes.")
            .arg(tree.GetNumActionNodes())
            .arg(tree.GetNumChanceNodes())
            .arg(tree.GetNumTerminalNodes())
            .arg(tree.GetNumShowdownNodes()));
    } catch (const std::exception& e) {
        appendToLog(QString("[ERROR] Failed to build tree: %1").arg(e.what()));
    }
}

MainWindow::~MainWindow() {
    stopSolving();
    if (solverThread_) {
        solverThread_->quit();
        solverThread_->wait();
    }
}

void MainWindow::estimateMemory() {
    appendToLog("Estimating trainable memory requirements...");
    try {
        QString boardText = boardInput->text().trimmed();
        QStringList cardTokens = boardText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        std::vector<int> initial_board_ints;
        for (const QString& tok : cardTokens) {
            auto val = poker_solver::core::Card::StringToInt(tok.toStdString());
            if (val) initial_board_ints.push_back(*val);
        }

        auto rule = buildRuleFromUI(initial_board_ints);
        poker_solver::tree::GameTree tree(rule);

        auto ip_hands = poker_solver::ranges::PrivateRangeConverter::StringToPrivateHands(
            ipRange_.toStdString(), initial_board_ints);
        auto oop_hands = poker_solver::ranges::PrivateRangeConverter::StringToPrivateHands(
            oopRange_.toStdString(), initial_board_ints);

        uint64_t bytes = tree.EstimateTrainableMemory(ip_hands.size(), oop_hands.size());
        double mb = bytes / (1024.0 * 1024.0);
        appendToLog(QString("Estimated trainable memory: %1 MB (%2 bytes) for %3 IP combos and %4 OOP combos.")
            .arg(mb, 0, 'f', 2)
            .arg(bytes)
            .arg(ip_hands.size())
            .arg(oop_hands.size()));
    } catch (const std::exception& e) {
        appendToLog(QString("[ERROR] Failed to estimate memory: %1").arg(e.what()));
    }
}

void MainWindow::startSolving() {
    appendToLog("Parsing inputs and starting solver...");
    startSolvingButton->setEnabled(false);

    try {
        QString boardText = boardInput->text().trimmed();
        QStringList cardTokens = boardText.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        std::vector<int> initial_board_ints;
        for (const QString& tok : cardTokens) {
            auto val = poker_solver::core::Card::StringToInt(tok.toStdString());
            if (val) initial_board_ints.push_back(*val);
        }

        auto rule = buildRuleFromUI(initial_board_ints);
        uint64_t initial_board_mask = poker_solver::core::Card::CardIntsToUint64(initial_board_ints);

        auto ip_hands = poker_solver::ranges::PrivateRangeConverter::StringToPrivateHands(
            ipRange_.toStdString(), initial_board_ints);
        auto oop_hands = poker_solver::ranges::PrivateRangeConverter::StringToPrivateHands(
            oopRange_.toStdString(), initial_board_ints);

        if (ip_hands.empty() || oop_hands.empty()) {
            throw std::runtime_error("IP or OOP range contains no valid hands for the given board.");
        }

        appendToLog(QString("Board: %1 (%2 cards), Pot: %3, Effective Stack: %4, IP combos: %5, OOP combos: %6")
            .arg(boardText)
            .arg(initial_board_ints.size())
            .arg(rule.initial_pot())
            .arg(rule.initial_effective_stack())
            .arg(ip_hands.size())
            .arg(oop_hands.size()));

        QString dictPath = "five_card_strength.txt";
        QString appDir = QCoreApplication::applicationDirPath();
        if (QFile::exists(appDir + "/five_card_strength.txt") || QFile::exists(appDir + "/five_card_strength.bin")) {
            dictPath = appDir + "/five_card_strength.txt";
        } else if (QFile::exists("test_data/five_card_strength.txt")) {
            dictPath = "test_data/five_card_strength.txt";
        } else if (QFile::exists("build/five_card_strength.txt")) {
            dictPath = "build/five_card_strength.txt";
        }
        auto evaluator = std::make_shared<poker_solver::eval::TableHandEvaluator>(dictPath.toStdString());
        auto phm = std::make_shared<poker_solver::ranges::PrivateHandManager>(
            std::vector<std::vector<poker_solver::core::PrivateHand>>{ip_hands, oop_hands},
            initial_board_mask
        );
        auto rrm = std::make_shared<poker_solver::ranges::RiverRangeManager>(evaluator);
        auto game_tree = std::make_shared<poker_solver::tree::GameTree>(rule);

        poker_solver::solver::CfrPlusSolver::Config config;
        config.iteration_limit = iterationsInput->text().toInt();
        if (config.iteration_limit <= 0) config.iteration_limit = 1000;
        config.num_threads = threadsInput->text().toInt();
        if (config.num_threads <= 0) config.num_threads = 8;

        config.target_exploitability = exploitabilityInput->text().trimmed().toDouble();
        config.exploitability_interval = 25;

        config.progress_callback = [this](int iter) {
            QMetaObject::invokeMethod(this, [this, iter]() {
                this->onSolverProgress(iter, "Solving...");
            }, Qt::QueuedConnection);
        };

        config.exploitability_callback = [this](int iter, double exp) {
            QMetaObject::invokeMethod(this, [this, iter, exp]() {
                this->appendToLog(QString("Iteration %1 - Exploitability: %2%").arg(iter).arg(exp, 0, 'f', 4));
            }, Qt::QueuedConnection);
        };

        auto solver = std::make_shared<poker_solver::solver::CfrPlusSolver>(game_tree, phm, rrm, rule, config);

        solverThread_ = new QThread(this);
        solverThread_->setStackSize(8 * 1024 * 1024); // 8 MB stack for CFR recursion
        solverWorker_ = new SolverWorker(solver);
        solverWorker_->moveToThread(solverThread_);

        connect(solverThread_, &QThread::started, solverWorker_, &SolverWorker::process);
        connect(solverWorker_, &SolverWorker::finishedStrategy, this, &MainWindow::onSolverFinishedStrategy);
        connect(solverWorker_, &SolverWorker::error, this, &MainWindow::onSolverError);
        connect(solverWorker_, &SolverWorker::finishedStrategy, solverWorker_, &QObject::deleteLater);
        connect(solverThread_, &QThread::finished, solverThread_, &QObject::deleteLater);

        solverThread_->start();

    } catch (const std::exception& e) {
        appendToLog(QString("[ERROR] Failed to start solver: %1").arg(e.what()));
        startSolvingButton->setEnabled(true);
    }
}

void MainWindow::stopSolving() {
    if (solverWorker_) {
        appendToLog("Stopping solver gracefully...");
        QMetaObject::invokeMethod(solverWorker_.data(), "stop", Qt::QueuedConnection);
    }
}

void MainWindow::onSolverProgress(int iteration, const QString& message) {
    appendToLog(QString("Iteration: %1").arg(iteration));
}

void MainWindow::onSolverFinished(const QString& strategy_str) {
    appendToLog("Solving complete! Strategy ready.");
    startSolvingButton->setEnabled(true);
    lastStrategy_ = nlohmann::json::parse(strategy_str.toStdString());
}

void MainWindow::onSolverFinishedStrategy(const QSharedPointer<nlohmann::json>& strategy_ptr) {
    appendToLog("Solving complete! Strategy is ready. Click 'Show Result' to explore.");
    startSolvingButton->setEnabled(true);
    if (strategy_ptr) {
        lastStrategy_ = *strategy_ptr;
    }
}

void MainWindow::onSolverError(const QString& err) {
    appendToLog("Solver Error: " + err);
    startSolvingButton->setEnabled(true);
}

void MainWindow::showResult() {
    appendToLog("Showing results...");
    if (lastStrategy_.is_null()) {
        appendToLog("No strategy available. Run the solver first.");
        return;
    }
    // Open the StrategyExplorer dialog as a modal window
    StrategyExplorer explorer(lastStrategy_, this);
    explorer.exec();
}

void MainWindow::loadResult() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open Strategy JSON", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) {
        return;
    }
    
    std::ifstream in(fileName.toStdString());
    if (!in.is_open()) {
        appendToLog("Failed to open file: " + fileName);
        return;
    }
    
    try {
        in >> lastStrategy_;
        appendToLog("Successfully loaded strategy from " + fileName);
        showResult();
    } catch (const std::exception& e) {
        appendToLog(QString("Failed to parse JSON: ") + e.what());
    }
}

void MainWindow::appendToLog(const QString& text) {
    logTextEdit->append(text);
}

void MainWindow::clearLog() {
    logTextEdit->clear();
} 