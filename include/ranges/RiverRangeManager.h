#ifndef POKER_SOLVER_RANGES_RIVER_RANGE_MANAGER_H_
#define POKER_SOLVER_RANGES_RIVER_RANGE_MANAGER_H_

#include "Card.h"                // For Card utilities
#include "compairer/Compairer.h" // For Compairer interface
#include "ranges/PrivateCards.h" // For PrivateCards
#include "ranges/RiverCombs.h"   // For RiverCombs struct
#include <cstdint>
#include <memory>    // For std::shared_ptr
#include <mutex>     // For std::mutex, std::lock_guard
#include <shared_mutex> // For std::shared_mutex
#include <stdexcept> // For exceptions
#include <unordered_map>
#include <vector>

namespace poker_solver {
namespace ranges {

// Manages the calculation and caching of evaluated hand strengths (ranks)
// for player ranges on specific river boards.
// This class is designed to be thread-safe for concurrent read/write access.
class RiverRangeManager {
public:
  // Constructor.
  // Args:
  //   compairer: A shared pointer to a concrete Compairer implementation
  //              used for hand evaluation. Must not be null.
  // Throws:
  //   std::invalid_argument if compairer is null.
  explicit RiverRangeManager(std::shared_ptr<core::Compairer> compairer);

  // Calculates or retrieves the cached vector of evaluated RiverCombs for a
  // given player, their initial range, and a specific river board.
  // The resulting vector is sorted by rank (worst hand first, i.e., highest
  // rank number first, consistent with the original project's sort order).
  // Args:
  //   player_index: The index of the player (0 or 1).
  //   initial_player_range: The vector of PrivateCards representing the
  //   player's
  //                         range *before* considering the river board.
  //   river_board_mask: The uint64_t bitmask of the 5 river board cards.
  // Returns:
  //   A const reference to the cached or newly calculated vector of RiverCombs.
  // Throws:
  //   std::out_of_range if player_index is invalid (currently only supports 0
  //   or 1). std::invalid_argument if river_board_mask doesn't represent
  //   exactly 5 cards.
  const std::vector<RiverCombs> &
  GetRiverCombos(size_t player_index,
                 const std::vector<core::PrivateCards> &initial_player_range,
                 uint64_t river_board_mask);

  // Overload that accepts the river board as a vector of card integers.
  // Throws:
  //   std::invalid_argument if river_board_ints contains invalid card integers
  //                         or does not represent exactly 5 cards.
  //   std::out_of_range if player_index is invalid.
  const std::vector<RiverCombs> &
  GetRiverCombos(size_t player_index,
                 const std::vector<core::PrivateCards> &initial_player_range,
                 const std::vector<int> &river_board_ints);

private:
  // Calculates, sorts, and caches the RiverCombs for a given player/board.
  // This is called internally by GetRiverCombos if the result isn't cached.
  // Assumes the cache lock is NOT held when called.
  // Returns a const reference to the newly inserted element in the cache.
  const std::vector<RiverCombs> &CalculateAndCacheRiverCombos(
      size_t player_index,
      const std::vector<core::PrivateCards> &initial_player_range,
      uint64_t river_board_mask);

  // The hand evaluator used to determine ranks.
  std::shared_ptr<core::Compairer> compairer_;

  // Caches are now thread_local in the cpp file to avoid any lock contention.

  // Deleted copy/move operations to prevent accidental copying of
  // caches/mutexes.
  RiverRangeManager(const RiverRangeManager &) = delete;
  RiverRangeManager &operator=(const RiverRangeManager &) = delete;
  RiverRangeManager(RiverRangeManager &&) = delete;
  RiverRangeManager &operator=(RiverRangeManager &&) = delete;
};

} // namespace ranges
} // namespace poker_solver

#endif // POKER_SOLVER_RANGES_RIVER_RANGE_MANAGER_H_
