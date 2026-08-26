#ifndef POKER_SOLVER_SOLVER_BEST_RESPONSE_CALCULATOR_H_
#define POKER_SOLVER_SOLVER_BEST_RESPONSE_CALCULATOR_H_

#include "GameTree.h"
#include "ranges/PrivateCardsManager.h"
#include "ranges/RiverRangeManager.h"
#include "Deck.h"
#include <vector>
#include <memory>
#include <span>

namespace poker_solver {
namespace solver {

class BestResponseCalculator {
public:
    BestResponseCalculator(
        std::shared_ptr<tree::GameTree> game_tree,
        std::shared_ptr<ranges::PrivateCardsManager> pcm,
        std::shared_ptr<ranges::RiverRangeManager> rrm,
        uint64_t initial_board_mask);

    double ComputeExploitability(const std::vector<std::vector<double>>& initial_reach_probs, double initial_pot = 2.0);

private:
    std::vector<double> br_utility(
        core::NodeRef node,
        std::vector<std::vector<std::vector<double>>>& stack,
        int depth,
        int br_player,
        uint64_t current_board_mask);

    std::vector<double> br_action_node(
        uint32_t node_idx,
        std::vector<std::vector<std::vector<double>>>& stack,
        int depth,
        int br_player,
        uint64_t current_board_mask);

    std::vector<double> br_chance_node(
        uint32_t node_idx,
        std::vector<std::vector<std::vector<double>>>& stack,
        int depth,
        int br_player,
        uint64_t current_board_mask);

    std::vector<double> br_showdown_node(
        uint32_t node_idx,
        std::vector<std::vector<std::vector<double>>>& stack,
        int depth,
        int br_player,
        uint64_t current_board_mask);

    std::vector<double> br_terminal_node(
        uint32_t node_idx,
        std::vector<std::vector<std::vector<double>>>& stack,
        int depth,
        int br_player,
        uint64_t current_board_mask);

    int GetHandIndex(int player, int c1, int c2) const;
    void ExchangeColor(double* utility, size_t num_hands, int player, int rank1, int rank2) const;

    std::shared_ptr<tree::GameTree> game_tree_;
    std::shared_ptr<ranges::PrivateCardsManager> pcm_;
    std::shared_ptr<ranges::RiverRangeManager> rrm_;
    uint64_t initial_board_mask_;
    const size_t num_players_ = 2;
};

} // namespace solver
} // namespace poker_solver

#endif // POKER_SOLVER_SOLVER_BEST_RESPONSE_CALCULATOR_H_
