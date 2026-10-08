#ifndef POKER_SOLVER_CORE_HAND_STRENGTH_EVALUATOR_H_
#define POKER_SOLVER_CORE_HAND_STRENGTH_EVALUATOR_H_

#include "poker_solver/core/poker_card.h"
#include <cstdint>
#include <vector>

namespace poker_solver {
namespace core {

enum class ComparisonResult { kPlayer1Wins, kPlayer2Wins, kTie };

// Abstract base class (interface) for poker hand evaluation and comparison.
class Compairer {
 public:
  virtual ~Compairer() = default;

  // Modern snake_case methods
  virtual ComparisonResult compare_hands(const std::vector<int>& private_hand1,
                                        const std::vector<int>& private_hand2,
                                        const std::vector<int>& public_board) const = 0;

  virtual ComparisonResult compare_hands(uint64_t private_mask1,
                                        uint64_t private_mask2,
                                        uint64_t public_mask) const = 0;

  virtual int get_hand_rank(const std::vector<int>& private_hand,
                            const std::vector<int>& public_board) const = 0;

  virtual int get_hand_rank(uint64_t private_mask, uint64_t public_mask) const = 0;

  // Backward-compatibility wrappers
  ComparisonResult CompareHands(const std::vector<int>& private_hand1,
                               const std::vector<int>& private_hand2,
                               const std::vector<int>& public_board) const {
    return compare_hands(private_hand1, private_hand2, public_board);
  }

  ComparisonResult CompareHands(uint64_t private_mask1,
                               uint64_t private_mask2,
                               uint64_t public_mask) const {
    return compare_hands(private_mask1, private_mask2, public_mask);
  }

  int GetHandRank(const std::vector<int>& private_hand,
                  const std::vector<int>& public_board) const {
    return get_hand_rank(private_hand, public_board);
  }

  int GetHandRank(uint64_t private_mask, uint64_t public_mask) const {
    return get_hand_rank(private_mask, public_mask);
  }
};

using HandStrengthEvaluator = Compairer;

} // namespace core
} // namespace poker_solver

#endif // POKER_SOLVER_CORE_HAND_STRENGTH_EVALUATOR_H_
