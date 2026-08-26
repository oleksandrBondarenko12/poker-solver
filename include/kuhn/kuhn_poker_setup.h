#ifndef POKER_SOLVER_KUHN_KUHN_POKER_SETUP_H_
#define POKER_SOLVER_KUHN_KUHN_POKER_SETUP_H_

#include <cstdint>
#include <vector>
#include <memory>
#include <stdexcept>

#include "compairer/Compairer.h"
#include "GameTree.h"
#include "ranges/PrivateCards.h"

namespace poker_solver {
namespace kuhn {

// --- Constants for Kuhn Poker ---
constexpr int KUHN_CARD_J = 0;
constexpr int KUHN_CARD_Q = 1;
constexpr int KUHN_CARD_K = 2;
constexpr int KUHN_DECK_SIZE = 3;

// Represent actions simply for Kuhn
enum class KuhnActionType { Check = 0, Bet = 1, Fold = 0, Call = 1 }; // Check/Fold=0, Bet/Call=1

// --- Kuhn Hand Comparer ---
class KuhnCompairer : public core::Compairer {
public:
    KuhnCompairer() = default;
    ~KuhnCompairer() override = default;

    // Compares two single Kuhn cards (higher card wins)
    core::ComparisonResult CompareHands(int private_card1, int private_card2) const;

    // --- Implementations matching the Compairer interface ---
    core::ComparisonResult CompareHands(
        const std::vector<int>& private_hand1,
        const std::vector<int>& private_hand2,
        const std::vector<int>& public_board) const override;

    core::ComparisonResult CompareHands(uint64_t private_mask1,
                                        uint64_t private_mask2,
                                        uint64_t public_mask) const override;

    int GetHandRank(const std::vector<int>& private_hand,
                    const std::vector<int>& public_board) const override;

    int GetHandRank(uint64_t private_mask,
                    uint64_t public_mask) const override;
};

// --- Kuhn Game Tree Builder ---
// Builds the Kuhn Poker game tree directly using the GameTree SoA structure.
std::shared_ptr<tree::GameTree> build_kuhn_game_tree();

// --- Helper: Kuhn Range ---
// Creates the initial uniform range {J, Q, K} for Kuhn Poker.
std::vector<core::PrivateCards> get_kuhn_initial_range(int dummy_card);

} // namespace kuhn
} // namespace poker_solver

#endif // POKER_SOLVER_KUHN_KUHN_POKER_SETUP_H_
