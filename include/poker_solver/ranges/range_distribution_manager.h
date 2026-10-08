#ifndef POKER_SOLVER_RANGES_RANGE_DISTRIBUTION_MANAGER_H_
#define POKER_SOLVER_RANGES_RANGE_DISTRIBUTION_MANAGER_H_

#include "poker_solver/ranges/hole_card_combination.h"
#include "poker_solver/core/poker_card.h"
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <optional>

namespace poker_solver {
namespace ranges {

// Manages initial private hand ranges for multiple players.
class PrivateCardsManager {
 public:
  PrivateCardsManager(
      std::vector<std::vector<core::PrivateCards>> initial_ranges,
      uint64_t initial_board_mask);

  // Modern snake_case methods
  size_t get_num_players() const { return num_players_; }
  size_t GetNumPlayers() const { return get_num_players(); }

  const std::vector<core::PrivateCards>& get_player_range(size_t player_index) const;
  const std::vector<core::PrivateCards>& GetPlayerRange(size_t player_index) const {
    return get_player_range(player_index);
  }

  std::optional<size_t> get_opponent_hand_index(size_t from_player_index,
                                               size_t to_player_index,
                                               size_t from_hand_index) const;
  std::optional<size_t> GetOpponentHandIndex(size_t from_player_index,
                                             size_t to_player_index,
                                             size_t from_hand_index) const {
    return get_opponent_hand_index(from_player_index, to_player_index, from_hand_index);
  }

  const std::vector<double>& get_initial_reach_probs(size_t player_index) const;
  const std::vector<double>& GetInitialReachProbs(size_t player_index) const {
    return get_initial_reach_probs(player_index);
  }

  void set_relative_probs(uint64_t initial_board_mask);
  void SetRelativeProbs(uint64_t initial_board_mask) {
    set_relative_probs(initial_board_mask);
  }

 private:
  void calculate_initial_reach_probs();

  size_t num_players_;
  uint64_t initial_board_mask_;
  std::vector<std::vector<core::PrivateCards>> player_ranges_;
  std::unordered_map<std::size_t, std::vector<std::optional<size_t>>> hand_hash_to_indices_;
  std::vector<std::vector<double>> initial_reach_probs_;
};

using RangeDistributionManager = PrivateCardsManager;

} // namespace ranges
} // namespace poker_solver

#endif // POKER_SOLVER_RANGES_RANGE_DISTRIBUTION_MANAGER_H_
