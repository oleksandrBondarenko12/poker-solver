#include "poker_solver/ranges/evaluated_river_combo.h" // Adjust path if necessary

namespace poker_solver {
namespace ranges {

// Default constructor implementation.
EvaluatedRiverCombo::EvaluatedRiverCombo() = default;

// Constructor implementation.
EvaluatedRiverCombo::EvaluatedRiverCombo(const core::PrivateHand& ph, int r,
                                         size_t original_idx)
    : private_hand(ph), rank(r), original_range_index(original_idx) {
}

} // namespace ranges
} // namespace poker_solver
