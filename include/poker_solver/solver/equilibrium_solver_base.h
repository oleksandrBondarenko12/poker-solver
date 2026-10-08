#ifndef POKER_SOLVER_SOLVER_EQUILIBRIUM_SOLVER_BASE_H_
#define POKER_SOLVER_SOLVER_EQUILIBRIUM_SOLVER_BASE_H_

#include <memory>
#include <string>
#include <vector>
#include "poker_solver/tree/extensive_game_tree.h"
#include "poker_solver/json.hpp"

using json = nlohmann::json;

namespace poker_solver {
namespace solver {

// Abstract base class for poker game solving algorithms.
class Solver {
 public:
  virtual ~Solver() = default;

  // Modern snake_case interface
  virtual void train() = 0;
  virtual void stop() = 0;
  virtual json dump_strategy(bool dump_evs, int max_depth = -1, int max_chance_outcomes = -1) const = 0;
  std::shared_ptr<tree::GameTree> get_game_tree() const { return game_tree_; }

  // Backward-compatibility wrappers
  void Train() { train(); }
  void Stop() { stop(); }
  json DumpStrategy(bool dump_evs, int max_depth = -1, int max_chance_outcomes = -1) const {
    return dump_strategy(dump_evs, max_depth, max_chance_outcomes);
  }
  std::shared_ptr<tree::GameTree> GetGameTree() const { return get_game_tree(); }

 protected:
  explicit Solver(std::shared_ptr<tree::GameTree> game_tree)
      : game_tree_(std::move(game_tree)) {}
  Solver() = default;

  std::shared_ptr<tree::GameTree> game_tree_;

 private:
  Solver(const Solver&) = delete;
  Solver& operator=(const Solver&) = delete;
  Solver(Solver&&) = delete;
  Solver& operator=(Solver&&) = delete;
};

using EquilibriumSolverBase = Solver;

} // namespace solver
} // namespace poker_solver

#endif // POKER_SOLVER_SOLVER_EQUILIBRIUM_SOLVER_BASE_H_
