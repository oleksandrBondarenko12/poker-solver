#ifndef POKER_SOLVER_KUHN_KUHN_POKER_SETUP_H_
#define POKER_SOLVER_KUHN_KUHN_POKER_SETUP_H_

#include <cstdint>
#include <vector>
#include <memory>
#include <stdexcept>

#include "poker_solver/compairer/hand_strength_evaluator.h"
#include "poker_solver/tree/extensive_game_tree.h"
#include "poker_solver/ranges/hole_card_combination.h"

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
    core::ComparisonResult compare_hands(int private_card1, int private_card2) const;
    core::ComparisonResult CompareHands(int private_card1, int private_card2) const {
        return compare_hands(private_card1, private_card2);
    }

    // --- Implementations matching the Compairer interface ---
    core::ComparisonResult compare_hands(
        const std::vector<int>& private_hand1,
        const std::vector<int>& private_hand2,
        const std::vector<int>& public_board) const override;

    core::ComparisonResult compare_hands(uint64_t private_mask1,
                                        uint64_t private_mask2,
                                        uint64_t public_mask) const override;

    int get_hand_rank(const std::vector<int>& private_hand,
                    const std::vector<int>& public_board) const override;

    int get_hand_rank(uint64_t private_mask,
                    uint64_t public_mask) const override;

    // Aliases
    core::ComparisonResult CompareHands(
        const std::vector<int>& private_hand1,
        const std::vector<int>& private_hand2,
        const std::vector<int>& public_board) const {
        return compare_hands(private_hand1, private_hand2, public_board);
    }

    core::ComparisonResult CompareHands(uint64_t private_mask1,
                                        uint64_t private_mask2,
                                        uint64_t public_mask) const {
        return compare_hands(private_mask1, private_mask2, public_mask);
    }

    int GetHandRank(const std::vector<int>& private_hand,
                    const std::vector<int>& public_board) const {
        return get_hand_rank(private_hand, public_board);
    }

    int GetHandRank(uint64_t private_mask,
                    uint64_t public_mask) const {
        return get_hand_rank(private_mask, public_mask);
    }
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
