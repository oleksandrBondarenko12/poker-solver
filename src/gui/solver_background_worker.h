#ifndef SOLVERWORKER_H
#define SOLVERWORKER_H

#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <memory>
#include "poker_solver/solver/public_chance_cfr_solver.h"

class SolverWorker : public QObject {
    Q_OBJECT

public:
    explicit SolverWorker(std::shared_ptr<poker_solver::solver::PCfrSolver> solver, QObject *parent = nullptr)
        : QObject(parent), solver_(solver) {}

public slots:
    void process() {
        try {
            solver_->Train();
            // Dump strategy with all 13 canonical ranks (A down to 2) on chance nodes
            auto json_ptr = QSharedPointer<nlohmann::json>::create(solver_->DumpStrategy(true, -1, 13));
            emit finishedStrategy(json_ptr);
        } catch (const std::exception& e) {
            emit error(QString::fromStdString(e.what()));
        }
    }

    void stop() {
        if (solver_) {
            solver_->Stop();
        }
    }

signals:
    void finishedStrategy(const QSharedPointer<nlohmann::json>& strategy_ptr);
    void finished(const QString& strategy_str);
    void error(const QString& err);

private:
    std::shared_ptr<poker_solver::solver::PCfrSolver> solver_;
};

#endif // SOLVERWORKER_H
