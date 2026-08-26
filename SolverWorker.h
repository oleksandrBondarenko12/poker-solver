#ifndef SOLVERWORKER_H
#define SOLVERWORKER_H

#include <QObject>
#include <QString>
#include <memory>
#include "solver/PCfrSolver.h"

class SolverWorker : public QObject {
    Q_OBJECT

public:
    explicit SolverWorker(std::shared_ptr<poker_solver::solver::PCfrSolver> solver, QObject *parent = nullptr)
        : QObject(parent), solver_(solver) {}

public slots:
    void process() {
        try {
            solver_->Train();
            auto json_data = solver_->DumpStrategy(true, 3);
            QString result = QString::fromStdString(json_data.dump());
            emit finished(result);
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
    void finished(const QString& strategy_str);
    void error(const QString& err);

private:
    std::shared_ptr<poker_solver::solver::PCfrSolver> solver_;
};

#endif // SOLVERWORKER_H
