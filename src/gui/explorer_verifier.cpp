#include "gto_strategy_explorer.h"
#include <QApplication>
#include <QFile>
#include <iostream>
#include <fstream>
#include "poker_solver/json.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    QString filename = "CustomGuiScenarioTest_actual_output.json";
    bool verify_only = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verify") {
            verify_only = true;
        } else if (!arg.empty() && arg[0] != '-') {
            filename = QString::fromUtf8(argv[i]);
        }
    }

    std::ifstream in(filename.toStdString());
    if (!in.is_open()) {
        std::cerr << "Failed to open output json: " << filename.toStdString() << std::endl;
        return 1;
    }
    
    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse json: " << e.what() << std::endl;
        return 1;
    }
    
    StrategyExplorer explorer(j, nullptr);
    
    if (verify_only) {
        bool ok = explorer.verifyChanceNodeStrategies();
        return ok ? 0 : 1;
    }

    explorer.show();
    return app.exec();
}
