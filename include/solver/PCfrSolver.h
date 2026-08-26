#ifndef POKER_SOLVER_SOLVER_PCFRSOLVER_H_
#define POKER_SOLVER_SOLVER_PCFRSOLVER_H_

#include "solver/Solver.h"          // Base class
#include "GameTree.h"               // For tree structure (ensure correct path)
#include "nodes/GameTreeNode.h"     // Node types and enums
#include "ranges/PrivateCardsManager.h" // Range management
#include "ranges/RiverRangeManager.h"   // River evaluation management
#include "compairer/Compairer.h"        // Hand evaluation
#include "Deck.h"                   // For deck info
#include "Card.h"                   // For Card utilities
#include "tools/Rule.h"             // For initial game state config

#include <vector>
#include <memory>
#include <string>
#include <atomic> // For stopping flag
#include <functional> // For std::function
#include <json.hpp> // Include actual json header

using json = nlohmann::json;

namespace poker_solver {
namespace solver {

class PCfrSolver : public Solver {
public:
    struct Config {
        int iteration_limit; 
        int num_threads;     
        std::function<void(int)> progress_callback; // Added for UI progress reporting

        Config() : iteration_limit(1000), num_threads(8), progress_callback(nullptr) {}
    };

    PCfrSolver(std::shared_ptr<tree::GameTree> game_tree,
               std::shared_ptr<ranges::PrivateCardsManager> pcm,
               std::shared_ptr<ranges::RiverRangeManager> rrm,
               const config::Rule& rule,
               Config solver_config = Config());

    void Train() override;
    void Stop() override;

    // Computes the Expected Value of the current average strategy and sets it in the trainables.
    // Returns the EV of the root node for both players.
    std::vector<double> ComputeEV();

    json DumpStrategy(bool dump_evs, int max_depth = -1) const override;

    std::shared_ptr<ranges::PrivateCardsManager> GetPrivateCardsManager() const { return pcm_; }
    std::shared_ptr<ranges::RiverRangeManager> GetRiverRangeManager() const { return rrm_; }
    uint64_t GetInitialBoardMask() const { return initial_board_mask_; }
    size_t GetNumPlayers() const { return num_players_; }

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
        int max_depth) const;

    void PreallocateTrainables(core::NodeRef node);

    void ExchangeColor(double* utility, size_t num_hands, int player, int rank1, int rank2) const;

    std::shared_ptr<ranges::PrivateCardsManager> pcm_;
    std::shared_ptr<ranges::RiverRangeManager> rrm_;
    uint64_t initial_board_mask_;
    core::Deck deck_; 
    Config config_;
    std::atomic<bool> stop_signal_{false};
    bool evs_calculated_ = false; 
    const size_t num_players_ = 2; 
};

} // namespace solver
} // namespace poker_solver

#endif // POKER_SOLVER_SOLVER_PCFRSOLVER_H_
