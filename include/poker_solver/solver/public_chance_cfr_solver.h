#ifndef POKER_SOLVER_SOLVER_PUBLIC_CHANCE_CFR_SOLVER_H_
#define POKER_SOLVER_SOLVER_PUBLIC_CHANCE_CFR_SOLVER_H_

#include "poker_solver/solver/equilibrium_solver_base.h"
#include "poker_solver/tree/extensive_game_tree.h"
#include "poker_solver/tree/game_tree_node_types.h"
#include "poker_solver/ranges/range_distribution_manager.h"
#include "poker_solver/ranges/river_evaluation_cache.h"
#include "poker_solver/compairer/hand_strength_evaluator.h"
#include "poker_solver/core/card_deck.h"
#include "poker_solver/core/poker_card.h"
#include "poker_solver/tree/scenario_game_rule.h"

#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include "poker_solver/json.hpp"

using json = nlohmann::json;

namespace poker_solver {
namespace solver {

class PCfrSolver : public Solver {
 public:
  struct Config {
    int iteration_limit;
    int num_threads;
    std::function<void(int)> progress_callback;
    std::function<void(int, double)> exploitability_callback;
    double target_exploitability;  // If > 0.0, stop when exploitability <= target_exploitability
    int exploitability_interval;   // Iteration interval to check exploitability (default: 25)

    Config()
        : iteration_limit(1000),
          num_threads(8),
          progress_callback(nullptr),
          exploitability_callback(nullptr),
          target_exploitability(0.0),
          exploitability_interval(25) {}
  };

  PCfrSolver(std::shared_ptr<tree::GameTree> game_tree,
             std::shared_ptr<ranges::PrivateCardsManager> pcm,
             std::shared_ptr<ranges::RiverRangeManager> rrm,
             const config::Rule& rule,
             Config solver_config = Config());

  // Modern snake_case methods
  void train() override;
  void stop() override;
  std::vector<double> compute_ev();
  double compute_exploitability() const;
  json dump_strategy(bool dump_evs, int max_depth = -1, int max_chance_outcomes = -1) const override;

  std::shared_ptr<ranges::PrivateCardsManager> get_private_cards_manager() const { return pcm_; }
  std::shared_ptr<ranges::RiverRangeManager> get_river_range_manager() const { return rrm_; }
  uint64_t get_initial_board_mask() const { return initial_board_mask_; }
  size_t get_num_players() const { return num_players_; }
  double get_last_exploitability() const { return last_exploitability_; }
  int get_completed_iterations() const { return completed_iterations_; }

  // Backward-compatibility wrappers
  void Train() { train(); }
  void Stop() { stop(); }
  std::vector<double> ComputeEV() { return compute_ev(); }
  double ComputeExploitability() const { return compute_exploitability(); }
  json DumpStrategy(bool dump_evs, int max_depth = -1, int max_chance_outcomes = -1) const {
    return dump_strategy(dump_evs, max_depth, max_chance_outcomes);
  }
  std::shared_ptr<ranges::PrivateCardsManager> GetPrivateCardsManager() const { return get_private_cards_manager(); }
  std::shared_ptr<ranges::RiverRangeManager> GetRiverRangeManager() const { return get_river_range_manager(); }
  uint64_t GetInitialBoardMask() const { return get_initial_board_mask(); }
  size_t GetNumPlayers() const { return get_num_players(); }

 private:
  void cfr_utility(
      core::NodeRef node,
      const double* reach_probs_0,
      const double* reach_probs_1,
      int traverser,
      int iteration,
      uint64_t current_board_mask,
      bool is_top_chance,
      bool use_average_strategy,
      double* out_utility);

  void cfr_action_node(
      uint32_t node_idx,
      const double* reach_probs_0,
      const double* reach_probs_1,
      int traverser,
      int iteration,
      uint64_t current_board_mask,
      bool is_top_chance,
      bool use_average_strategy,
      double* out_utility);

  void cfr_chance_node(
      uint32_t node_idx,
      const double* reach_probs_0,
      const double* reach_probs_1,
      int traverser,
      int iteration,
      uint64_t current_board_mask,
      bool is_top_chance,
      bool use_average_strategy,
      double* out_utility);

  void cfr_showdown_node(
      uint32_t node_idx,
      const double* reach_probs_0,
      const double* reach_probs_1,
      int traverser,
      uint64_t current_board_mask,
      double* out_utility);

  void cfr_terminal_node(
      uint32_t node_idx,
      const double* reach_probs_0,
      const double* reach_probs_1,
      int traverser,
      uint64_t current_board_mask,
      double* out_utility);

  json dump_strategy_recursive(
      core::NodeRef node,
      bool dump_evs,
      int current_depth,
      int max_depth,
      uint64_t current_board_mask,
      int max_chance_outcomes = -1) const;

  void preallocate_trainables(core::NodeRef node);
  void exchange_color(double* utility, size_t num_hands, int player, int rank1, int rank2) const;

  std::shared_ptr<ranges::PrivateCardsManager> pcm_;
  std::shared_ptr<ranges::RiverRangeManager> rrm_;
  uint64_t initial_board_mask_;
  core::Deck deck_; 
  Config config_;
  std::atomic<bool> stop_signal_{false};
  bool evs_calculated_ = false; 
  const size_t num_players_ = 2; 
  double initial_pot_ = 2.0;
  double last_exploitability_ = 100.0;
  int completed_iterations_ = 0;
};

using PublicChanceCfrSolver = PCfrSolver;

} // namespace solver
} // namespace poker_solver

#endif // POKER_SOLVER_SOLVER_PUBLIC_CHANCE_CFR_SOLVER_H_
