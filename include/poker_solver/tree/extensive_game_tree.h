#ifndef POKER_SOLVER_TREE_EXTENSIVE_GAME_TREE_H_
#define POKER_SOLVER_TREE_EXTENSIVE_GAME_TREE_H_

#include "poker_solver/core/card_deck.h"
#include "poker_solver/tree/poker_action_edge.h"
#include "poker_solver/tree/game_tree_node_types.h"
#include "poker_solver/tree/tree_building_config.h"
#include "poker_solver/tree/scenario_game_rule.h"
#include <cstdint>
#include "poker_solver/json.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <span>
#include <mutex>

namespace poker_solver {
namespace solver {
class Trainable;
}
namespace ranges {
class PrivateCardsManager;
}
}

namespace poker_solver {
namespace tree {
class GameTree;
}
namespace kuhn {
std::shared_ptr<tree::GameTree> build_kuhn_game_tree();
}
namespace core {

struct Edge {
    core::GameAction action;
    core::NodeRef child;
};
}
namespace tree {

// Represents extensive-form game tree using Structure of Arrays (SoA).
class GameTree {
 public:
  GameTree(const std::string &json_filepath, const core::Deck &deck);
  explicit GameTree(const config::Rule &rule);
  explicit GameTree(const core::Deck &deck) : deck_(deck) {}

  // --- Accessors ---
  core::NodeRef get_root() const { return root_; }
  const core::Deck &get_deck() const { return deck_; }
  core::NodeRef GetRoot() const { return get_root(); }
  const core::Deck &GetDeck() const { return get_deck(); }

  // --- Tree Analysis ---
  void calculate_tree_metadata();
  void print_tree(int max_depth = -1) const;
  uint64_t estimate_trainable_memory(size_t p0_range_size, size_t p1_range_size) const;

  void CalculateTreeMetadata() { calculate_tree_metadata(); }
  void PrintTree(int max_depth = -1) const { print_tree(max_depth); }
  uint64_t EstimateTrainableMemory(size_t p0, size_t p1) const { return estimate_trainable_memory(p0, p1); }

  // --- Action Nodes ---
  inline size_t get_num_action_nodes() const { return action_players_.size(); }
  inline uint8_t get_action_player(uint32_t idx) const { return action_players_[idx]; }
  inline core::GameRound get_action_round(uint32_t idx) const { return action_rounds_[idx]; }
  inline double get_action_pot(uint32_t idx) const { return action_pots_[idx]; }
  inline std::span<const core::Edge> get_action_edges(uint32_t idx) const {
      return std::span<const core::Edge>(action_edges_.data() + action_first_edge_[idx], action_num_edges_[idx]);
  }
  std::shared_ptr<solver::Trainable> get_trainable(uint32_t action_idx, uint64_t canonical_board_mask, size_t num_actions, size_t num_hands);
  void set_trainable(uint32_t action_idx, uint64_t canonical_board_mask, std::shared_ptr<solver::Trainable> trainable);

  inline size_t GetNumActionNodes() const { return get_num_action_nodes(); }
  inline uint8_t GetActionPlayer(uint32_t idx) const { return get_action_player(idx); }
  inline core::GameRound GetActionRound(uint32_t idx) const { return get_action_round(idx); }
  inline double GetActionPot(uint32_t idx) const { return get_action_pot(idx); }
  inline std::span<const core::Edge> GetActionEdges(uint32_t idx) const { return get_action_edges(idx); }
  std::shared_ptr<solver::Trainable> GetTrainable(uint32_t action_idx, uint64_t canonical_board_mask, size_t num_actions, size_t num_hands) {
    return get_trainable(action_idx, canonical_board_mask, num_actions, num_hands);
  }
  void SetTrainable(uint32_t action_idx, uint64_t canonical_board_mask, std::shared_ptr<solver::Trainable> trainable) {
    set_trainable(action_idx, canonical_board_mask, trainable);
  }
  
  // --- Chance Nodes ---
  inline size_t get_num_chance_nodes() const { return chance_rounds_.size(); }
  inline core::GameRound get_chance_round(uint32_t idx) const { return chance_rounds_[idx]; }
  inline double get_chance_pot(uint32_t idx) const { return chance_pots_[idx]; }
  inline core::NodeRef get_chance_child(uint32_t idx) const { return chance_children_[idx]; }
  inline std::span<const core::Card> get_chance_cards(uint32_t idx) const {
      return std::span<const core::Card>(chance_cards_.data() + chance_first_card_[idx], chance_num_cards_[idx]);
  }

  inline size_t GetNumChanceNodes() const { return get_num_chance_nodes(); }
  inline core::GameRound GetChanceRound(uint32_t idx) const { return get_chance_round(idx); }
  inline double GetChancePot(uint32_t idx) const { return get_chance_pot(idx); }
  inline core::NodeRef GetChanceChild(uint32_t idx) const { return get_chance_child(idx); }
  inline std::span<const core::Card> GetChanceCards(uint32_t idx) const { return get_chance_cards(idx); }

  // --- Terminal Nodes ---
  inline size_t get_num_terminal_nodes() const { return terminal_pots_.size(); }
  inline double get_terminal_pot(uint32_t idx) const { return terminal_pots_[idx]; }
  inline std::span<const double> get_terminal_payoffs(uint32_t idx) const {
      return std::span<const double>(terminal_payoffs_.data() + (idx * 2), 2);
  }

  inline size_t GetNumTerminalNodes() const { return get_num_terminal_nodes(); }
  inline double GetTerminalPot(uint32_t idx) const { return get_terminal_pot(idx); }
  inline std::span<const double> GetTerminalPayoffs(uint32_t idx) const { return get_terminal_payoffs(idx); }

  // --- Showdown Nodes ---
  inline size_t get_num_showdown_nodes() const { return showdown_pots_.size(); }
  inline double get_showdown_pot(uint32_t idx) const { return showdown_pots_[idx]; }
  inline std::span<const double> get_showdown_payoffs(uint32_t idx) const {
      return std::span<const double>(showdown_payoffs_.data() + (idx * 2), 2);
  }

  inline size_t GetNumShowdownNodes() const { return get_num_showdown_nodes(); }
  inline double GetShowdownPot(uint32_t idx) const { return get_showdown_pot(idx); }
  inline std::span<const double> GetShowdownPayoffs(uint32_t idx) const { return get_showdown_payoffs(idx); }

  // --- Node Creation Helpers ---
  core::NodeRef create_action_node(uint8_t player, core::GameRound round, double pot);
  core::NodeRef create_chance_node(core::GameRound round, double pot, const std::vector<core::Card>& dealt_cards);
  core::NodeRef create_terminal_node(double pot, const std::vector<double>& payoffs);
  core::NodeRef create_showdown_node(core::GameRound round, double pot, const std::vector<double>& payoffs);
  void build_action_node(uint32_t node_idx, const std::vector<core::GameAction>& actions, const std::vector<core::NodeRef>& children);
  void set_root(core::NodeRef root) { root_ = root; }

  core::NodeRef CreateActionNode(uint8_t player, core::GameRound round, double pot) { return create_action_node(player, round, pot); }
  core::NodeRef CreateChanceNode(core::GameRound round, double pot, const std::vector<core::Card>& dealt_cards) { return create_chance_node(round, pot, dealt_cards); }
  core::NodeRef CreateTerminalNode(double pot, const std::vector<double>& payoffs) { return create_terminal_node(pot, payoffs); }
  core::NodeRef CreateShowdownNode(core::GameRound round, double pot, const std::vector<double>& payoffs) { return create_showdown_node(round, pot, payoffs); }
  void BuildActionNode(uint32_t node_idx, const std::vector<core::GameAction>& actions, const std::vector<core::NodeRef>& children) { build_action_node(node_idx, actions, children); }
  void SetRoot(core::NodeRef root) { set_root(root); }

 private:
  void build_branch(core::NodeRef current_node,
                    config::Rule current_rule,
                    const core::GameAction &last_action, int actions_this_round,
                    int raises_this_street);

  void build_chance_node(uint32_t node_idx, const config::Rule &rule);

  void build_action_node_internal(uint32_t node_idx,
                                 config::Rule current_rule_state,
                                 const core::GameAction &last_action,
                                 int actions_this_round, int raises_this_street);

  std::vector<double>
  get_possible_bets(const config::Rule &rule, size_t player_index,
                   double current_player_commit, double opponent_commit,
                   double effective_stack, const core::GameAction &last_action,
                   double pot_before_action, core::GameRound round) const;

  static double round_bet(double amount, double min_bet_increment);

  // SoA Node Storage
  alignas(64) std::vector<uint8_t> action_players_;
  alignas(64) std::vector<core::GameRound> action_rounds_;
  alignas(64) std::vector<double> action_pots_;
  alignas(64) std::vector<uint32_t> action_first_edge_;
  alignas(64) std::vector<uint16_t> action_num_edges_;
  alignas(64) std::vector<core::Edge> action_edges_;
  
  mutable std::mutex trainables_mutex_;
  std::vector<std::unordered_map<uint64_t, std::shared_ptr<solver::Trainable>>> action_trainables_;

  alignas(64) std::vector<core::GameRound> chance_rounds_;
  alignas(64) std::vector<double> chance_pots_;
  alignas(64) std::vector<core::NodeRef> chance_children_;
  alignas(64) std::vector<uint32_t> chance_first_card_;
  alignas(64) std::vector<uint16_t> chance_num_cards_;
  alignas(64) std::vector<core::Card> chance_cards_;

  alignas(64) std::vector<double> terminal_pots_;
  alignas(64) std::vector<double> terminal_payoffs_;

  alignas(64) std::vector<double> showdown_pots_;
  alignas(64) std::vector<double> showdown_payoffs_;
  
  core::NodeRef root_ = core::kNullNode;
  core::Deck deck_;
  std::optional<config::Rule> build_rule_;

  GameTree(const GameTree &) = delete;
  GameTree &operator=(const GameTree &) = delete;
  GameTree(GameTree &&) = delete;
  GameTree &operator=(GameTree &&) = delete;
};

using ExtensiveGameTree = GameTree;

} // namespace tree
} // namespace poker_solver

#endif // POKER_SOLVER_TREE_EXTENSIVE_GAME_TREE_H_
