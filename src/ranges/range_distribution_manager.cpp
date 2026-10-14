#include "poker_solver/ranges/range_distribution_manager.h" // Adjust path if necessary
#include "poker_solver/core/poker_card.h" // Needed for Card utilities used within

#include <cmath>      // For std::abs
#include <functional> // For std::hash
#include <iomanip>    // For std::fixed / std::setprecision (optional)
#include <iostream>   // For std::cout / std::cerr
#include <numeric>    // For std::accumulate
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map> // Included for hand_hash_to_indices_
#include <vector>

// Use aliases for namespaces
namespace core = poker_solver::core;

namespace poker_solver {
namespace ranges {

PrivateHandManager::PrivateHandManager(
    std::vector<std::vector<core::PrivateHand>> initial_ranges,
    uint64_t initial_board_mask)
    : num_players_(initial_ranges.size()),
      initial_board_mask_(initial_board_mask),
      player_ranges_(std::move(initial_ranges)), // Use move constructor
      initial_reach_probs_(num_players_) {

  if (num_players_ == 0) {
    throw std::invalid_argument("Initial ranges cannot be empty.");
  }
  // Currently, relative probability calculation only supports 2 players.
  if (num_players_ != 2) {
    throw std::invalid_argument(
        "PrivateHandManager currently only supports exactly 2 players "
        "for initial reach probability calculation.");
  }

  // --- Build the Hand Hash Lookup Table ---
  std::hash<core::PrivateHand> hasher;
  for (size_t player_idx = 0; player_idx < num_players_; ++player_idx) {
    const auto &range = player_ranges_[player_idx];
    for (size_t hand_idx = 0; hand_idx < range.size(); ++hand_idx) {
      const core::PrivateHand &hand = range[hand_idx];
      std::size_t hand_hash = hasher(hand);

      // Ensure map entry exists for this hash
      // Use try_emplace for potentially better efficiency if hash exists
      auto [it, inserted] = hand_hash_to_indices_.try_emplace(
          hand_hash,
          std::vector<std::optional<size_t>>(num_players_, std::nullopt));

      // Check for duplicate hands within the same player's range (optional but
      // recommended)
      if (it->second[player_idx].has_value()) { // Check if index already set
        std::ostringstream oss;
        oss << "Duplicate hand found in range for player " << player_idx
            << " at index " << hand_idx << " and "
            << it->second[player_idx].value() << " (Hand: " << hand.ToString()
            << ")";
        // Depending on requirements, could throw or just warn
        std::cerr << "[WARNING PCM] " << oss.str() << std::endl;
        // If throwing is desired: throw std::invalid_argument(oss.str());
      }

      // Store the index for this player and hand hash
      it->second[player_idx] = hand_idx;
    }
  }

  // --- Calculate Initial Reach Probabilities ---
  calculate_initial_reach_probs();
}

const std::vector<core::PrivateHand> &
PrivateHandManager::get_player_range(size_t player_index) const {
  if (player_index >= num_players_) {
    std::ostringstream oss;
    oss << "Invalid player index: " << player_index << ". Must be less than "
        << num_players_ << ".";
    throw std::out_of_range(oss.str());
  }
  return player_ranges_[player_index];
}

std::optional<size_t>
PrivateHandManager::get_opponent_hand_index(size_t from_player_index,
                                             size_t to_player_index,
                                             size_t from_hand_index) const {

  if (from_player_index >= num_players_ || to_player_index >= num_players_ ||
      from_hand_index >= player_ranges_[from_player_index].size()) {
    return std::nullopt; // Invalid input indices
  }

  const core::PrivateHand &source_hand =
      player_ranges_[from_player_index][from_hand_index];
  std::hash<core::PrivateHand> hasher;
  std::size_t hand_hash = hasher(source_hand);

  auto it = hand_hash_to_indices_.find(hand_hash);
  if (it == hand_hash_to_indices_.end()) {
    // This might happen if a hand exists for one player but not the other
    // (e.g., asymmetric ranges after filtering or initial setup)
    return std::nullopt;
  }

  // Return the optional index stored for the target player
  // Check bounds just in case vector wasn't initialized correctly (shouldn't
  // happen)
  if (to_player_index < it->second.size()) {
    return it->second[to_player_index];
  } else {
    return std::nullopt;
  }
}

// Calculates initial reach probabilities considering initial board card
// removal. Hands conflicting with the initial board are assigned 0 probability.
// Valid hand weights are normalized so that initial reach probabilities sum
// to 1.0 for each player.
void PrivateHandManager::calculate_initial_reach_probs() {
  if (num_players_ != 2)
    return; // Only implemented for 2 players

  std::vector<double> weight_sums(num_players_, 0.0);

  for (size_t player_id = 0; player_id < num_players_; ++player_id) {
    const auto &player_range = player_ranges_[player_id];
    initial_reach_probs_[player_id].assign(player_range.size(), 0.0);

    for (size_t i = 0; i < player_range.size(); ++i) {
      const core::PrivateHand &player_hand = player_range[i];
      uint64_t player_mask = player_hand.GetBoardMask();

      // Skip hands conflicting with the initial board
      if (core::Card::DoBoardsOverlap(player_mask, initial_board_mask_)) {
        initial_reach_probs_[player_id][i] = 0.0;
        continue;
      }

      double weight = player_hand.Weight();
      initial_reach_probs_[player_id][i] = weight;
      weight_sums[player_id] += weight;
    }
  }

  // Normalize the probabilities for each player
  for (size_t player_id = 0; player_id < num_players_; ++player_id) {
    double total_sum = weight_sums[player_id];
    if (total_sum > 1e-12) { // Use tolerance for floating point comparison
      for (double &prob : initial_reach_probs_[player_id]) {
        prob /= total_sum;
      }
    } else {
      // Handle case where no hands are possible (e.g., board conflicts with all
      // ranges)
      if (!player_ranges_[player_id].empty()) {
        std::cerr
            << "[WARNING PCM] Player " << player_id
            << " weight sum is zero, resulting in zero reach probabilities."
            << std::endl;
      }
      if (!initial_reach_probs_[player_id].empty()) {
        std::fill(initial_reach_probs_[player_id].begin(),
                  initial_reach_probs_[player_id].end(), 0.0);
      }
    }

    // *** ADDED DEBUG LOGGING ***
    double check_sum =
        std::accumulate(initial_reach_probs_[player_id].begin(),
                        initial_reach_probs_[player_id].end(), 0.0);
    if (std::abs(check_sum - 1.0) > 1e-6 && check_sum > 1e-12) {
      std::cerr << "[ERROR PCM] Player " << player_id
                << " initial reach probs do not sum to 1.0. Sum = " << check_sum
                << std::endl;
    } else if (std::abs(check_sum) < 1e-12 &&
               initial_reach_probs_[player_id].size() > 0 &&
               total_sum > 1e-12) {
      std::cerr << "[WARNING PCM] Player " << player_id
                << " initial reach probs sum to 0.0 after normalization."
                << std::endl;
    } else {
      std::cout << "[DEBUG PCM] Player " << player_id
                << " initial reach probs sum: " << std::fixed
                << std::setprecision(8) << check_sum << std::endl;
    }
    // ***************************
  }

  valid_deal_mass_ = calculate_valid_deal_mass(initial_reach_probs_);
  for (int s1 = 0; s1 < 4; ++s1) {
    for (int s2 = 0; s2 < 4; ++s2) {
      suit_pair_symmetric_[s1][s2] = compute_suit_pair_symmetry(s1, s2);
    }
  }
}

double PrivateHandManager::calculate_valid_deal_mass(
    const std::vector<std::vector<double>> &reach_probs) const {
  if (num_players_ != 2 || reach_probs.size() < 2)
    return 1.0;

  const auto &range_0 = player_ranges_[0];
  const auto &range_1 = player_ranges_[1];
  double p1_sum = 0.0;
  std::array<double, 52> p1_card_sum{};
  for (size_t h1 = 0; h1 < range_1.size(); ++h1) {
    if (core::Card::DoBoardsOverlap(range_1[h1].GetBoardMask(), initial_board_mask_))
      continue;
    double prob1 = (h1 < reach_probs[1].size()) ? reach_probs[1][h1] : 0.0;
    p1_sum += prob1;
    p1_card_sum[range_1[h1].Card1Int()] += prob1;
    p1_card_sum[range_1[h1].Card2Int()] += prob1;
  }

  double valid_deal_mass = 0.0;
  for (size_t h0 = 0; h0 < range_0.size(); ++h0) {
    if (core::Card::DoBoardsOverlap(range_0[h0].GetBoardMask(), initial_board_mask_))
      continue;
    auto oppo_idx = get_opponent_hand_index(0, 1, h0);
    double same_hand_prob =
        (oppo_idx && *oppo_idx < reach_probs[1].size()) ? reach_probs[1][*oppo_idx] : 0.0;
    double compat_p1 = p1_sum - p1_card_sum[range_0[h0].Card1Int()] -
                       p1_card_sum[range_0[h0].Card2Int()] + same_hand_prob;
    double prob0 = (h0 < reach_probs[0].size()) ? reach_probs[0][h0] : 0.0;
    valid_deal_mass += prob0 * compat_p1;
  }

  return valid_deal_mass;
}

bool PrivateHandManager::compute_suit_pair_symmetry(int suit1, int suit2) const {
  if (suit1 == suit2)
    return true;
  if (num_players_ != 2)
    return false;

  for (size_t p = 0; p < num_players_; ++p) {
    const auto &range = player_ranges_[p];
    const auto &probs = initial_reach_probs_[p];
    if (range.size() != probs.size())
      return false;

    std::vector<double> hand_probs(52 * 52, -1.0);
    for (size_t i = 0; i < range.size(); ++i) {
      int c1 = range[i].Card1Int();
      int c2 = range[i].Card2Int();
      if (c1 > c2)
        std::swap(c1, c2);
      hand_probs[c1 * 52 + c2] = probs[i];
    }

    for (size_t i = 0; i < range.size(); ++i) {
      int c1 = range[i].Card1Int();
      int c2 = range[i].Card2Int();
      int s1 = c1 % 4, r1 = c1 / 4;
      int s2 = c2 % 4, r2 = c2 / 4;
      if (s1 == suit1)
        s1 = suit2;
      else if (s1 == suit2)
        s1 = suit1;
      if (s2 == suit1)
        s2 = suit2;
      else if (s2 == suit2)
        s2 = suit1;

      int mc1 = r1 * 4 + s1;
      int mc2 = r2 * 4 + s2;
      if (mc1 > mc2)
        std::swap(mc1, mc2);

      double partner_prob = hand_probs[mc1 * 52 + mc2];
      if (partner_prob < 0.0 || std::abs(partner_prob - probs[i]) > 1e-7) {
        return false;
      }
    }
  }
  return true;
}

const std::vector<double> &
PrivateHandManager::get_initial_reach_probs(size_t player_index) const {
  if (player_index >= num_players_) {
    std::ostringstream oss;
    oss << "Invalid player index: " << player_index << ". Must be less than "
        << num_players_ << ".";
    throw std::out_of_range(oss.str());
  }
  // This vector should have been calculated and normalized in the constructor
  return initial_reach_probs_[player_index];
}

void PrivateHandManager::set_relative_probs(uint64_t initial_board_mask) {
  size_t players = player_ranges_.size();
  for (size_t p = 0; p < players; ++p) {
    size_t oppo = 1 - p;
    double player_prob_sum = 0.0;

    for (size_t i = 0; i < player_ranges_[p].size(); ++i) {
      double oppo_prob_sum = 0.0;
      auto &player_card = player_ranges_[p][i];
      uint64_t player_long = player_card.GetBoardMask();

      if (core::Card::DoBoardsOverlap(player_long, initial_board_mask)) {
        continue;
      }

      for (const auto &oppo_card : player_ranges_[oppo]) {
        uint64_t oppo_long = oppo_card.GetBoardMask();
        if (core::Card::DoBoardsOverlap(oppo_long, initial_board_mask) ||
            core::Card::DoBoardsOverlap(oppo_long, player_long)) {
          continue;
        }
        oppo_prob_sum += oppo_card.Weight();
      }
      player_card.SetRelativeProb(oppo_prob_sum * player_card.Weight());
      player_prob_sum += player_card.GetRelativeProb();
    }
    for (size_t i = 0; i < player_ranges_[p].size(); ++i) {
      if (player_prob_sum > 1e-12) {
        player_ranges_[p][i].SetRelativeProb(
            player_ranges_[p][i].GetRelativeProb() / player_prob_sum);
      }
    }
  }
}

} // namespace ranges
} // namespace poker_solver
