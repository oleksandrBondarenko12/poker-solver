#ifndef POKER_SOLVER_RANGES_EVALUATED_RIVER_COMBO_H_
#define POKER_SOLVER_RANGES_EVALUATED_RIVER_COMBO_H_

#include "poker_solver/ranges/hole_card_combination.h" // For PrivateCards
#include <cstddef> // For size_t

namespace poker_solver {
namespace ranges {

// Represents an evaluated hand combination on the river.
// Stores the original private cards, their evaluated rank on the river board,
// and the index they held in the player's initial range.
struct EvaluatedRiverCombo {
  // Default constructor (creates an invalid state).
  EvaluatedRiverCombo();

  // Constructs an EvaluatedRiverCombo object.
  // Args:
  //   private_hand: The PrivateHand object representing the hole cards.
  //   rank: The evaluated rank of the 7-card hand (private + river board).
  //         Lower rank values are typically better.
  //   original_range_index: The index this hand occupied in the player's
  //                         initial range vector (before river filtering).
  EvaluatedRiverCombo(const core::PrivateHand& private_hand, int rank,
                      size_t original_range_index);

  // The player's private hole cards for this specific combination.
  core::PrivateHand private_hand;

  // The evaluated rank (strength) of the 7-card hand (private + river board).
  // Lower values indicate stronger hands. A value like kInvalidRank from the
  // evaluator might indicate an error or impossible hand.
  int rank = -1; // Initialize to an invalid rank

  // The original index of this hand within the player's pre-river range vector.
  // Useful for mapping results back or calculating reach probabilities.
  size_t original_range_index = static_cast<size_t>(-1); // Initialize to invalid index
};

} // namespace ranges
} // namespace poker_solver

#endif // POKER_SOLVER_RANGES_EVALUATED_RIVER_COMBO_H_
