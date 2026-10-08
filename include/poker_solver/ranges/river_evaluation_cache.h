#ifndef POKER_SOLVER_RANGES_RIVER_EVALUATION_CACHE_H_
#define POKER_SOLVER_RANGES_RIVER_EVALUATION_CACHE_H_

#include "poker_solver/core/poker_card.h"
#include "poker_solver/compairer/hand_strength_evaluator.h"
#include "poker_solver/ranges/hole_card_combination.h"
#include "poker_solver/ranges/evaluated_river_combo.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace poker_solver {
namespace ranges {

// Manages calculation and caching of evaluated hand strengths for player ranges on river boards.
class RiverRangeManager {
 public:
  explicit RiverRangeManager(std::shared_ptr<core::Compairer> compairer);

  // Modern snake_case methods
  const std::vector<RiverCombs>& get_river_combos(
      size_t player_index,
      const std::vector<core::PrivateCards>& initial_player_range,
      uint64_t river_board_mask);

  const std::vector<RiverCombs>& get_river_combos(
      size_t player_index,
      const std::vector<core::PrivateCards>& initial_player_range,
      const std::vector<int>& river_board_ints);

  // Backward-compatibility aliases
  const std::vector<RiverCombs>& GetRiverCombos(
      size_t player_index,
      const std::vector<core::PrivateCards>& initial_player_range,
      uint64_t river_board_mask) {
    return get_river_combos(player_index, initial_player_range, river_board_mask);
  }

  const std::vector<RiverCombs>& GetRiverCombos(
      size_t player_index,
      const std::vector<core::PrivateCards>& initial_player_range,
      const std::vector<int>& river_board_ints) {
    return get_river_combos(player_index, initial_player_range, river_board_ints);
  }

 private:
  const std::vector<RiverCombs>& calculate_and_cache_river_combos(
      size_t player_index,
      const std::vector<core::PrivateCards>& initial_player_range,
      uint64_t river_board_mask);

  std::shared_ptr<core::Compairer> compairer_;

  RiverRangeManager(const RiverRangeManager&) = delete;
  RiverRangeManager& operator=(const RiverRangeManager&) = delete;
  RiverRangeManager(RiverRangeManager&&) = delete;
  RiverRangeManager& operator=(RiverRangeManager&&) = delete;
};

using RiverEvaluationCache = RiverRangeManager;

} // namespace ranges
} // namespace poker_solver

#endif // POKER_SOLVER_RANGES_RIVER_EVALUATION_CACHE_H_
