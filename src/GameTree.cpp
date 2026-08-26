#include "GameTree.h"
#include "Card.h"
#include "tools/StreetSetting.h"
#include "trainable/Trainable.h"
#include "trainable/DiscountedCfrTrainable.h"

#include <algorithm>
#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <span>
#include <mutex>
#include <memory>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <set>

using json = nlohmann::json;
namespace core = poker_solver::core;
namespace config = poker_solver::config;
namespace solver = poker_solver::solver;

namespace poker_solver {
namespace tree {

// --- Tree Node Creation Helpers ---

core::NodeRef GameTree::CreateActionNode(uint8_t player, core::GameRound round, double pot) {
    uint32_t idx = action_players_.size();
    action_players_.push_back(player);
    action_rounds_.push_back(round);
    action_pots_.push_back(pot);
    action_first_edge_.push_back(0);
    action_num_edges_.push_back(0);
    action_trainables_.push_back({});
    return {core::GameTreeNodeType::kAction, idx};
}

core::NodeRef GameTree::CreateChanceNode(core::GameRound round, double pot, const std::vector<core::Card>& dealt_cards) {
    uint32_t idx = chance_rounds_.size();
    chance_rounds_.push_back(round);
    chance_pots_.push_back(pot);
    chance_children_.push_back(core::kNullNode);
    chance_first_card_.push_back(chance_cards_.size());
    chance_num_cards_.push_back(dealt_cards.size());
    for (const auto& c : dealt_cards) {
        chance_cards_.push_back(c);
    }
    return {core::GameTreeNodeType::kChance, idx};
}

core::NodeRef GameTree::CreateTerminalNode(double pot, const std::vector<double>& payoffs) {
    uint32_t idx = terminal_pots_.size();
    terminal_pots_.push_back(pot);
    terminal_payoffs_.push_back(payoffs.size() > 0 ? payoffs[0] : 0.0);
    terminal_payoffs_.push_back(payoffs.size() > 1 ? payoffs[1] : 0.0);
    return {core::GameTreeNodeType::kTerminal, idx};
}

core::NodeRef GameTree::CreateShowdownNode(core::GameRound round, double pot, const std::vector<double>& payoffs) {
    showdown_pots_.push_back(pot);
    showdown_payoffs_.push_back(payoffs.size() > 0 ? payoffs[0] : 0.0);
    showdown_payoffs_.push_back(payoffs.size() > 1 ? payoffs[1] : 0.0);
    return {core::GameTreeNodeType::kShowdown, static_cast<uint32_t>(showdown_pots_.size() - 1)};
}

void GameTree::BuildActionNode(uint32_t node_idx, const std::vector<core::GameAction>& actions, const std::vector<core::NodeRef>& children) {
    if (actions.size() != children.size()) {
        throw std::invalid_argument("actions and children size mismatch in BuildActionNode");
    }
    action_first_edge_[node_idx] = action_edges_.size();
    action_num_edges_[node_idx] = actions.size();
    for (size_t i = 0; i < actions.size(); ++i) {
        action_edges_.push_back({actions[i], children[i]});
    }
}

std::shared_ptr<solver::Trainable> GameTree::GetTrainable(uint32_t action_idx, uint64_t canonical_board_mask, size_t num_actions, size_t num_hands) {
    std::lock_guard<std::mutex> lock(trainables_mutex_);
    if (action_idx >= action_trainables_.size()) {
        action_trainables_.resize(action_idx + 1);
    }
    auto it = action_trainables_[action_idx].find(canonical_board_mask);
    if (it != action_trainables_[action_idx].end()) {
        return it->second;
    }
    
    // Allocate on-the-fly
    auto trainable = std::make_shared<solver::DiscountedCfrTrainable>(num_actions, num_hands);
    action_trainables_[action_idx][canonical_board_mask] = trainable;
    return trainable;
}

void GameTree::SetTrainable(uint32_t action_idx, uint64_t canonical_board_mask, std::shared_ptr<solver::Trainable> trainable) {
    std::lock_guard<std::mutex> lock(trainables_mutex_);
    if (action_idx >= action_trainables_.size()) {
        action_trainables_.resize(action_idx + 1);
    }
    action_trainables_[action_idx][canonical_board_mask] = trainable;
}

// --- Constructor (Dynamic Building) ---

GameTree::GameTree(const config::Rule& rule)
    : deck_(rule.GetDeck()), build_rule_(rule) {

  size_t starting_player = 1; // Default to OOP acting first postflop
  if (rule.GetStartingRound() == core::GameRound::kPreflop) {
    std::cout << "[INFO] Building Preflop Game Tree. IP (player 0 / SB) acts first." << std::endl;
    starting_player = 0;
  }

  root_ = CreateActionNode(starting_player, rule.GetStartingRound(), rule.GetInitialPot());

  std::cout << "Starting GameTree build branch..." << std::endl;
  BuildBranch(root_, rule, core::GameAction(core::PokerAction::kRoundBegin), 0, 0);
  std::cout << "Finished GameTree build branch." << std::endl;
  CalculateTreeMetadata();
}

GameTree::GameTree(const std::string &json_filepath, const core::Deck &deck) : deck_(deck) {
  throw std::logic_error("JSON loading with flat SoA tree is not implemented yet.");
}

void GameTree::BuildBranch(core::NodeRef current_node,
                           config::Rule current_rule,
                           const core::GameAction& last_action,
                           int actions_this_round, int raises_this_street) {
  if (current_node == core::kNullNode) return;

  switch (current_node.type) {
  case core::GameTreeNodeType::kAction:
    BuildActionNode(current_node.index, current_rule, last_action, actions_this_round, raises_this_street);
    break;
  case core::GameTreeNodeType::kChance:
    BuildChanceNode(current_node.index, current_rule);
    break;
  case core::GameTreeNodeType::kShowdown:
  case core::GameTreeNodeType::kTerminal:
    break;
  }
}

void GameTree::BuildChanceNode(uint32_t node_idx, const config::Rule& rule_at_chance_creation) {
  core::GameRound round_completed_by_this_chance_deal = chance_rounds_[node_idx];
  double current_pot = chance_pots_[node_idx];
  double stack = rule_at_chance_creation.GetInitialEffectiveStack();

  double ip_commit = rule_at_chance_creation.GetInitialCommitment(0);
  double oop_commit = rule_at_chance_creation.GetInitialCommitment(1);
  constexpr double eps = 1e-9;

  const bool ip_is_all_in = (stack - ip_commit) <= eps;
  const bool oop_is_all_in = (stack - oop_commit) <= eps;
  const bool effectively_all_in_runout = ip_is_all_in && oop_is_all_in;

  core::NodeRef child_node_after_this_deal = core::kNullNode;

  if (round_completed_by_this_chance_deal == core::GameRound::kRiver) {
    child_node_after_this_deal = CreateShowdownNode(core::GameRound::kRiver, current_pot, {ip_commit, oop_commit});
  } else if (effectively_all_in_runout) {
    core::GameRound round_for_next_deal = static_cast<core::GameRound>(static_cast<int>(round_completed_by_this_chance_deal) + 1);
    child_node_after_this_deal = CreateChanceNode(round_for_next_deal, current_pot, {});
  } else {
    core::GameRound round_for_action_node = round_completed_by_this_chance_deal;
    size_t next_player_to_act = 1;
    child_node_after_this_deal = CreateActionNode(next_player_to_act, round_for_action_node, current_pot);
  }

  chance_children_[node_idx] = child_node_after_this_deal;
  BuildBranch(child_node_after_this_deal, rule_at_chance_creation, core::GameAction(core::PokerAction::kRoundBegin), 0, 0);
}

void tree::GameTree::BuildActionNode(uint32_t node_idx,
    config::Rule current_rule_state,
    const core::GameAction& last_action, int actions_this_round,
    int raises_this_street) {
  
  size_t current_player = action_players_[node_idx];
  size_t opponent_player = 1 - current_player;
  double current_player_commit = current_rule_state.GetInitialCommitment(current_player);
  double opponent_commit = current_rule_state.GetInitialCommitment(opponent_player);
  double pot_before_action = action_pots_[node_idx];
  double stack = current_rule_state.GetInitialEffectiveStack();
  double player_stack_remaining = stack - current_player_commit;
  core::GameRound current_round = action_rounds_[node_idx];

  uint32_t first_edge_idx = action_edges_.size();

  if (player_stack_remaining <= 1e-9 && opponent_commit > current_player_commit) {
    action_first_edge_[node_idx] = first_edge_idx;
    action_num_edges_[node_idx] = 0;
    return;
  } else if (player_stack_remaining <= 1e-9) {
    action_first_edge_[node_idx] = first_edge_idx;
    action_num_edges_[node_idx] = 0;
    return;
  }

  constexpr double eps = 1e-9;
  bool can_check = std::abs(current_player_commit - opponent_commit) < eps;
  bool can_call = (opponent_commit - current_player_commit) > eps;
  double opp_remaining_stack = stack - opponent_commit;
  bool opponent_is_all_in = opp_remaining_stack <= eps;
  bool can_fold = can_call;
  bool can_bet_or_raise = !opponent_is_all_in && (player_stack_remaining > eps) && (raises_this_street < current_rule_state.GetRaiseLimitPerStreet());

  struct RecursiveCall {
    core::NodeRef child_node;
    config::Rule next_rule;
    core::GameAction action;
    int next_actions_this_round;
    int next_raises_this_street;
  };
  std::vector<RecursiveCall> recursive_calls;

  // 1. Check Action
  if (can_check) {
    core::GameAction check_action(core::PokerAction::kCheck);
    core::NodeRef child_node_after_check;
    bool round_ends = (actions_this_round > 0);
    if (round_ends) {
      if (current_round == core::GameRound::kRiver) {
        child_node_after_check = CreateShowdownNode(current_round, pot_before_action, {current_rule_state.GetInitialCommitment(0), current_rule_state.GetInitialCommitment(1)});
      } else {
        core::GameRound round_for_next_deal = static_cast<core::GameRound>(static_cast<int>(current_round) + 1);
        child_node_after_check = CreateChanceNode(round_for_next_deal, pot_before_action, {});
      }
    } else {
      child_node_after_check = CreateActionNode(opponent_player, current_round, pot_before_action);
    }
    action_edges_.push_back({check_action, child_node_after_check});
    recursive_calls.push_back({child_node_after_check, current_rule_state, check_action, actions_this_round + 1, raises_this_street});
  }

  // 2. Call Action
  if (can_call) {
    core::GameAction call_action(core::PokerAction::kCall);
    double amount_to_call = opponent_commit - current_player_commit;
    double actual_call_amount = std::min(amount_to_call, player_stack_remaining);
    double next_pot = pot_before_action + actual_call_amount;
    double next_player_commit = current_player_commit + actual_call_amount;

    core::NodeRef child_node_after_call;
    bool called_player_is_now_all_in = (player_stack_remaining - actual_call_amount) <= eps;
    bool all_in_by_call = called_player_is_now_all_in || opponent_is_all_in;

    if (current_round == core::GameRound::kRiver) {
      std::vector<double> final_commitments_for_showdown;
      if (current_player == 0) {
        final_commitments_for_showdown = {next_player_commit, opponent_commit};
      } else {
        final_commitments_for_showdown = {opponent_commit, next_player_commit};
      }
      child_node_after_call = CreateShowdownNode(core::GameRound::kRiver, next_pot, final_commitments_for_showdown);
    } else {
      bool preflop_bb_option = (current_round == core::GameRound::kPreflop && actions_this_round == 0);
      if (preflop_bb_option || all_in_by_call) {
          if (preflop_bb_option && !all_in_by_call) {
              child_node_after_call = CreateActionNode(opponent_player, current_round, next_pot);
          } else {
              core::GameRound round_for_next_deal = static_cast<core::GameRound>(static_cast<int>(current_round) + 1);
              child_node_after_call = CreateChanceNode(round_for_next_deal, next_pot, {});
          }
      } else {
          core::GameRound round_for_next_deal = static_cast<core::GameRound>(static_cast<int>(current_round) + 1);
          child_node_after_call = CreateChanceNode(round_for_next_deal, next_pot, {});
      }
    }
    action_edges_.push_back({call_action, child_node_after_call});
    
    config::Rule next_rule_call = current_rule_state;
    if (current_player == 0) next_rule_call.SetInitialIpCommit(next_player_commit);
    else next_rule_call.SetInitialOopCommit(next_player_commit);
    
    recursive_calls.push_back({child_node_after_call, next_rule_call, call_action, actions_this_round + 1, raises_this_street});
  }

  // 3. Fold Action
  if (can_fold) {
    core::GameAction fold_action(core::PokerAction::kFold);
    std::vector<double> payoffs(2);
    payoffs[current_player] = -current_player_commit;
    payoffs[opponent_player] = current_player_commit;

    core::NodeRef child_node_after_fold = CreateTerminalNode(pot_before_action, payoffs);
    action_edges_.push_back({fold_action, child_node_after_fold});
  }

  // 4. Bet / Raise Actions
  if (can_bet_or_raise) {
    bool is_facing_action = opponent_commit > current_player_commit + eps;
    bool is_raise = is_facing_action;
    auto action_type = is_raise ? core::PokerAction::kRaise : core::PokerAction::kBet;

    std::vector<double> bet_amounts = GetPossibleBets(current_rule_state, current_player, current_player_commit, opponent_commit, stack, last_action, pot_before_action, current_round);

    for (double total_amount : bet_amounts) {
      double actual_bet_or_raise_value = total_amount;
      if (is_raise) {
        actual_bet_or_raise_value = total_amount - (opponent_commit - current_player_commit);
        if (actual_bet_or_raise_value < eps) continue;
      }

      core::GameAction bet_raise_action(action_type, actual_bet_or_raise_value);
      double next_pot = pot_before_action + total_amount;
      double next_player_commit = current_player_commit + total_amount;

      core::NodeRef child_node = CreateActionNode(opponent_player, current_round, next_pot);
      action_edges_.push_back({bet_raise_action, child_node});

      config::Rule next_rule_bet_raise = current_rule_state;
      if (current_player == 0) next_rule_bet_raise.SetInitialIpCommit(next_player_commit);
      else next_rule_bet_raise.SetInitialOopCommit(next_player_commit);
      
      recursive_calls.push_back({child_node, next_rule_bet_raise, bet_raise_action, actions_this_round + 1, raises_this_street + 1});
    }
  }

  action_first_edge_[node_idx] = first_edge_idx;
  action_num_edges_[node_idx] = action_edges_.size() - first_edge_idx;

  for (const auto& call : recursive_calls) {
    BuildBranch(call.child_node, call.next_rule, call.action, call.next_actions_this_round, call.next_raises_this_street);
  }
}

std::vector<double> tree::GameTree::GetPossibleBets(
    const config::Rule& rule, size_t player_index, double current_player_commit,
    double opponent_commit, double effective_stack,
    const core::GameAction& last_action, double pot_before_action,
    core::GameRound round) const {
  const config::GameTreeBuildingSettings& settings = rule.GetBuildSettings();
  const config::StreetSetting& street_setting = settings.GetSetting(player_index, round);

  std::vector<double> size_ratios;
  bool allow_all_in = street_setting.allow_all_in;
  bool is_raise = opponent_commit > current_player_commit;
  bool is_donk = (player_index == 1 && round > core::GameRound::kPreflop &&
                  (last_action.GetAction() == core::PokerAction::kRoundBegin || last_action.GetAction() == core::PokerAction::kCheck) &&
                  !street_setting.donk_sizes_percent.empty());

  if (is_donk) size_ratios = street_setting.donk_sizes_percent;
  else if (is_raise) size_ratios = street_setting.raise_sizes_percent;
  else size_ratios = street_setting.bet_sizes_percent;

  std::set<double> possible_amounts;
  double stack_remaining = effective_stack - current_player_commit;
  if (stack_remaining <= 1e-9) return {};

  double call_amount = is_raise ? (opponent_commit - current_player_commit) : 0.0;
  double min_bet_size = rule.GetBigBlind();

  for (double ratio_percent : size_ratios) {
    if (ratio_percent <= 0) continue;
    double ratio = ratio_percent / 100.0;
    double calculated_size = is_raise ? (ratio * (pot_before_action + call_amount)) : (ratio * pot_before_action);
    
    double final_size = is_raise ? std::max(calculated_size, std::max(min_bet_size, call_amount)) : std::max(calculated_size, min_bet_size);
    double rounded_size = RoundBet(final_size, rule.GetSmallBlind());
    double amount_to_add = is_raise ? (call_amount + rounded_size) : rounded_size;
    amount_to_add = std::min(amount_to_add, stack_remaining);

    if (amount_to_add > 1e-9) {
      if (!is_raise || amount_to_add > call_amount + 1e-9) {
        possible_amounts.insert(amount_to_add);
      }
    }
  }

  if (allow_all_in && stack_remaining > 1e-9) {
    bool all_in_covered = (!possible_amounts.empty() && std::abs(*possible_amounts.rbegin() - stack_remaining) < 1e-9);
    if (!all_in_covered) {
      bool is_valid_all_in = (!is_raise) || (stack_remaining > call_amount + 1e-9);
      if (is_valid_all_in) possible_amounts.insert(stack_remaining);
    }
  }

  return std::vector<double>(possible_amounts.begin(), possible_amounts.end());
}

double tree::GameTree::RoundBet(double amount, double min_bet_increment) {
  if (min_bet_increment <= 1e-9) return amount;
  return std::max(min_bet_increment, std::round(amount / min_bet_increment) * min_bet_increment);
}

void tree::GameTree::CalculateTreeMetadata() {
  // Can be used for depth/size info, but requires array traversal.
}

void tree::GameTree::PrintTree(int max_depth) const {
  std::cout << "[PrintTree] Flat SoA representation. Root index: " << root_.index << "\n";
  // Simplified for now
}

uint64_t tree::GameTree::EstimateTrainableMemory(size_t p0_range_size, size_t p1_range_size) const {
  return 0; // Simplified
}

} // namespace tree
} // namespace poker_solver
