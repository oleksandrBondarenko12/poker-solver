#ifndef POKER_SOLVER_CONFIG_RULE_H_
#define POKER_SOLVER_CONFIG_RULE_H_

#include "poker_solver/core/card_deck.h"                          // For Deck
#include "poker_solver/tree/tree_building_config.h" // For GameTreeBuildingSettings
#include "poker_solver/tree/game_tree_node_types.h"                // For GameRound
#include <vector>
#include <cstddef> // For size_t
#include <stdexcept> // For exceptions
#include <sstream>   // For error messages
#include <utility>   // For std::move

namespace poker_solver {
namespace config {

class Rule {
 public:
  Rule(const core::Deck& deck,
       double initial_oop_commit,
       double initial_ip_commit,
       core::GameRound starting_round,
       const std::vector<int>& initial_board_cards, // <<< ADDED PARAMETER
       int raise_limit_per_street,
       double small_blind,
       double big_blind,
       double initial_effective_stack,
       const GameTreeBuildingSettings& build_settings,
       double all_in_threshold_ratio = 0.98);

  // --- Modern snake_case Accessors ---
  const core::Deck& deck() const { return deck_; }
  double initial_oop_commit() const { return initial_oop_commit_; }
  double initial_ip_commit() const { return initial_ip_commit_; }
  core::GameRound starting_round() const { return starting_round_; }
  const std::vector<int>& initial_board_cards_int() const { return initial_board_cards_int_; }
  int raise_limit_per_street() const { return raise_limit_per_street_; }
  double small_blind() const { return small_blind_; }
  double big_blind() const { return big_blind_; }
  double initial_effective_stack() const { return initial_effective_stack_; }
  const GameTreeBuildingSettings& build_settings() const { return build_settings_; }
  double all_in_threshold_ratio() const { return all_in_threshold_ratio_; }
  double initial_pot() const;
  double initial_commitment(size_t player_index) const;

  // --- Modern snake_case Modifiers ---
  void set_initial_oop_commit(double amount) { initial_oop_commit_ = amount; }
  void set_initial_ip_commit(double amount) { initial_ip_commit_ = amount; }

  // --- Backward-compatibility Accessors ---
  const core::Deck& GetDeck() const { return deck(); }
  double GetInitialOopCommit() const { return initial_oop_commit(); }
  double GetInitialIpCommit() const { return initial_ip_commit(); }
  core::GameRound GetStartingRound() const { return starting_round(); }
  const std::vector<int>& GetInitialBoardCardsInt() const { return initial_board_cards_int(); }
  int GetRaiseLimitPerStreet() const { return raise_limit_per_street(); }
  double GetSmallBlind() const { return small_blind(); }
  double GetBigBlind() const { return big_blind(); }
  double GetInitialEffectiveStack() const { return initial_effective_stack(); }
  const GameTreeBuildingSettings& GetBuildSettings() const { return build_settings(); }
  double GetAllInThresholdRatio() const { return all_in_threshold_ratio(); }
  double GetInitialPot() const { return initial_pot(); }
  double GetInitialCommitment(size_t player_index) const { return initial_commitment(player_index); }

  // --- Backward-compatibility Modifiers ---
  void SetInitialOopCommit(double amount) { set_initial_oop_commit(amount); }
  void SetInitialIpCommit(double amount) { set_initial_ip_commit(amount); }

 private:
  core::Deck deck_;
  double initial_oop_commit_;
  double initial_ip_commit_;
  core::GameRound starting_round_;
  std::vector<int> initial_board_cards_int_; // Stores the initial board cards
  int raise_limit_per_street_;
  double small_blind_;
  double big_blind_;
  double initial_effective_stack_;
  GameTreeBuildingSettings build_settings_;
  double all_in_threshold_ratio_;

  const std::vector<size_t> players_ = {0, 1};
};

using ScenarioGameRule = Rule;

} // namespace config
} // namespace poker_solver

#endif // POKER_SOLVER_CONFIG_RULE_H_
