#include "poker_solver/ranges/river_evaluation_cache.h" // Adjust path if necessary
#include "poker_solver/core/poker_card.h"                     // For Card utilities
#include "poker_solver/compairer/table_hand_evaluator.h"  // For kInvalidRank - TODO: move constant?

#include <algorithm> // For std::sort
#include <bit>       // For std::popcount (C++20) - include if using
#include <iostream>  // For std::cout, std::cerr
#include <sstream>
#include <stdexcept>
#include <utility> // For std::move
#include <vector>

// Use aliases for namespaces
namespace core = poker_solver::core;
namespace eval = poker_solver::eval; // For kInvalidRank

namespace poker_solver {
namespace ranges {

// Thread-local caches to completely eliminate lock contention
static thread_local std::unordered_map<uint64_t, std::vector<RiverCombs>> tl_player_0_cache;
static thread_local std::unordered_map<uint64_t, std::vector<RiverCombs>> tl_player_1_cache;

// --- Constructor ---

RiverRangeManager::RiverRangeManager(std::shared_ptr<core::Compairer> compairer)
    : compairer_(std::move(compairer)) { // Use std::move
  if (!compairer_) {
    throw std::invalid_argument("Compairer cannot be null.");
  }
}

// --- Public Methods ---

const std::vector<RiverCombs> &RiverRangeManager::get_river_combos(
    size_t player_index,
    const std::vector<core::PrivateCards> &initial_player_range,
    uint64_t river_board_mask) {

  // Select the appropriate cache based on player index
  auto &cache = (player_index == 0) ? tl_player_0_cache : tl_player_1_cache;

  if (player_index > 1) { // Basic check for 2 players
    std::ostringstream oss;
    oss << "Invalid player index for RiverRangeManager: " << player_index;
    throw std::out_of_range(oss.str());
  }

  // --- Cache Lookup (Lock-Free Thread-Local) ---
  auto it = cache.find(river_board_mask);
  if (it != cache.end()) {
    return it->second;
  }

  // --- DEBUG LOGGING ---
  // std::cout << "[RRM_GET] Cache MISS. Board: 0x" << std::hex <<
  // river_board_mask << std::dec << ". Calculating..." << std::endl;
  // --- END DEBUG ---

  // --- Not found in cache, calculate it ---
  // Call the private method to do the calculation and caching
  return calculate_and_cache_river_combos(player_index, initial_player_range,
                                      river_board_mask);
}

// Overload for vector<int> board input
const std::vector<RiverCombs> &RiverRangeManager::get_river_combos(
    size_t player_index,
    const std::vector<core::PrivateCards> &initial_player_range,
    const std::vector<int> &river_board_ints) {

  // Convert board vector to mask, includes validation via CardIntsToUint64
  uint64_t river_board_mask;
  try {
    river_board_mask = core::Card::CardIntsToUint64(river_board_ints);
  } catch (const std::out_of_range &e) {
    std::ostringstream oss;
    oss << "Invalid card integer in river board vector: " << e.what();
    throw std::invalid_argument(oss.str());
  }
  // Delegate to the mask version
  return get_river_combos(player_index, initial_player_range, river_board_mask);
}

// --- Private Calculation and Caching Method ---

const std::vector<RiverCombs> &RiverRangeManager::calculate_and_cache_river_combos(
    size_t player_index,
    const std::vector<core::PrivateCards> &initial_player_range,
    uint64_t river_board_mask) {

  // --- DEBUG LOGGING ---
  // std::cout << "[RRM_CALC] P" << player_index << " Board: 0x" << std::hex <<
  // river_board_mask << std::dec
  //       << " InitialRangeSize: " << initial_player_range.size() << std::endl;
  // --- END DEBUG ---

  // Validate board mask represents 5 cards
  int pop_count = 0;
#if defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L
  pop_count = std::popcount(river_board_mask);
#elif defined(__GNUC__) || defined(__clang__)
  pop_count = __builtin_popcountll(river_board_mask);
#else
  // Fallback popcount for older compilers
  uint64_t temp_mask = river_board_mask;
  while (temp_mask > 0) {
    temp_mask &= (temp_mask - 1);
    pop_count++;
  }
#endif

  if (pop_count != 5) {
    std::ostringstream oss;
    oss << "River board mask must represent exactly 5 cards. Mask: 0x"
        << std::hex << river_board_mask << " has " << std::dec << pop_count
        << " cards.";
    throw std::invalid_argument(oss.str());
  }

  std::vector<RiverCombs> calculated_combos;
  calculated_combos.reserve(
      initial_player_range.size()); // Pre-allocate approx size

  for (size_t i = 0; i < initial_player_range.size(); ++i) {
    const core::PrivateCards &hand = initial_player_range[i];
    uint64_t private_mask = hand.GetBoardMask();

    // Skip hands that conflict with the river board
    if (core::Card::DoBoardsOverlap(private_mask, river_board_mask)) {
      // --- DEBUG LOGGING ---
      // std::cout << "[RRM_CALC] Skipping hand " << hand.ToString()
      //           << " (Idx " << i << ") due to board conflict with 0x" <<
      //           std::hex << river_board_mask << std::dec << std::endl;
      // --- END DEBUG ---
      continue;
    }

    // Evaluate the 7-card hand rank using the injected compairer
    int rank = compairer_->GetHandRank(private_mask, river_board_mask);

    // --- DEBUG LOGGING ---
    // std::cout << "[RRM_CALC] Hand: " << hand.ToString()
    //           << " (Orig Idx " << i << "), Rank: " << rank << std::endl;
    // --- END DEBUG ---

    calculated_combos.emplace_back(hand, rank, i);
  }

  // --- DEBUG LOGGING ---
  // std::cout << "[RRM_CALC] Calculated combos size before sort: "
  //           << calculated_combos.size() << " for Board: 0x" << std::hex <<
  //           river_board_mask << std::dec << std::endl;
  // --- END DEBUG ---

  // Sort the results by rank.
  // NOTE: Sorting descending by rank number (worst hand first).
  std::sort(
      calculated_combos.begin(), calculated_combos.end(),
      [](const RiverCombs &lhs, const RiverCombs &rhs) {
        // Treat invalid ranks as the absolute worst
        bool lhs_invalid =
            (lhs.rank == eval::Dic5Compairer::kInvalidRank || lhs.rank == -1);
        bool rhs_invalid =
            (rhs.rank == eval::Dic5Compairer::kInvalidRank || rhs.rank == -1);

        if (lhs_invalid && rhs_invalid)
          return false; // Equal (both invalid)
        if (lhs_invalid)
          return true; // Invalid is "greater" (worse) than valid
        if (rhs_invalid)
          return false; // Valid is "less" (better) than invalid

        // Both valid, sort descending by rank number (worst first)
        return lhs.rank > rhs.rank;
      });

  // --- Cache Insertion (Lock-Free Thread-Local) ---
  // Select cache again
  auto &cache = (player_index == 0) ? tl_player_0_cache : tl_player_1_cache;
  auto result = cache.emplace(river_board_mask, std::move(calculated_combos));
  return result.first->second;
}

} // namespace ranges
} // namespace poker_solver
