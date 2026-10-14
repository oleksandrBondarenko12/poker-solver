#ifndef POKER_SOLVER_RANGES_RANGE_DISTRIBUTION_MANAGER_H_
#define POKER_SOLVER_RANGES_RANGE_DISTRIBUTION_MANAGER_H_

#include "poker_solver/ranges/hole_card_combination.h"
#include "poker_solver/core/poker_card.h"
#include <array>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <optional>

namespace poker_solver {
namespace ranges {

// Manages initial private hand ranges for multiple players.
class PrivateHandManager {
 public:
  PrivateHandManager(
      std::vector<std::vector<core::PrivateHand>> initial_ranges,
      uint64_t initial_board_mask);

  // Modern snake_case methods
  size_t get_num_players() const { return num_players_; }
  size_t GetNumPlayers() const { return get_num_players(); }

  const std::vector<core::PrivateHand>& get_player_range(size_t player_index) const;
  const std::vector<core::PrivateHand>& GetPlayerRange(size_t player_index) const {
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

  bool is_suit_pair_symmetric(int suit1, int suit2) const {
    if (suit1 == suit2) return true;
    if (suit1 < 0 || suit1 >= 4 || suit2 < 0 || suit2 >= 4) return false;
    return suit_pair_symmetric_[suit1][suit2];
  }
  bool IsSuitPairSymmetric(int suit1, int suit2) const {
    return is_suit_pair_symmetric(suit1, suit2);
  }

  const std::vector<int>& get_suit_swap_indices(size_t player_index, int suit1, int suit2) const {
    return suit_swap_indices_[player_index][suit1][suit2];
  }
  const std::vector<int>& GetSuitSwapIndices(size_t player_index, int suit1, int suit2) const {
    return get_suit_swap_indices(player_index, suit1, suit2);
  }

  double calculate_valid_deal_mass(
      const std::vector<std::vector<double>> &reach_probs) const;
  double CalculateValidDealMass(
      const std::vector<std::vector<double>> &reach_probs) const {
    return calculate_valid_deal_mass(reach_probs);
  }

  double get_valid_deal_mass() const { return valid_deal_mass_; }
  double GetValidDealMass() const { return get_valid_deal_mass(); }

 private:
  void calculate_initial_reach_probs();
  bool compute_suit_pair_symmetry(int suit1, int suit2) const;
  void compute_suit_swap_indices();

  size_t num_players_;
  uint64_t initial_board_mask_;
  std::vector<std::vector<core::PrivateHand>> player_ranges_;
  std::unordered_map<std::size_t, std::vector<std::optional<size_t>>> hand_hash_to_indices_;
  std::vector<std::vector<double>> initial_reach_probs_;
  double valid_deal_mass_ = 1.0;
  std::array<std::array<bool, 4>, 4> suit_pair_symmetric_{};
  std::vector<std::array<std::array<std::vector<int>, 4>, 4>> suit_swap_indices_;
};

using RangeDistributionManager = PrivateHandManager;

} // namespace ranges
} // namespace poker_solver

#endif // POKER_SOLVER_RANGES_RANGE_DISTRIBUTION_MANAGER_H_
