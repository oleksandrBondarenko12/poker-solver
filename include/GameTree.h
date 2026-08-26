#ifndef POKER_SOLVER_TREE_GAME_TREE_H_
#define POKER_SOLVER_TREE_GAME_TREE_H_

#include "Deck.h"
#include "nodes/GameActions.h"
#include "nodes/GameTreeNode.h"
#include "tools/GameTreeBuildingSettings.h"
#include "tools/Rule.h"
#include <cstdint>
#include <json.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <span>
#include <mutex>
#include <memory>

// Forward declarations for Trainable type
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

// Represents the entire game tree for a specific poker scenario using SoA and flat indices.
class GameTree {
public:
  // Constructor for loading a pre-built tree from a JSON file.
  GameTree(const std::string &json_filepath, const core::Deck &deck);

  // Constructor for building the tree dynamically based on rules.
  explicit GameTree(const config::Rule &rule);

  // Constructor for building the tree manually
  explicit GameTree(const core::Deck &deck) : deck_(deck) {}

  // --- Accessors ---
  core::NodeRef GetRoot() const { return root_; }
  const core::Deck &GetDeck() const { return deck_; }

  // --- Tree Analysis ---
  void CalculateTreeMetadata();
  void PrintTree(int max_depth = -1) const;
  uint64_t EstimateTrainableMemory(size_t p0_range_size, size_t p1_range_size) const;

  // --- Flat Structure Accessors ---

  // Action Nodes
  inline size_t GetNumActionNodes() const { return action_players_.size(); }
  inline uint8_t GetActionPlayer(uint32_t idx) const { return action_players_[idx]; }
  inline core::GameRound GetActionRound(uint32_t idx) const { return action_rounds_[idx]; }
  inline double GetActionPot(uint32_t idx) const { return action_pots_[idx]; }
  inline std::span<const core::Edge> GetActionEdges(uint32_t idx) const {
      return std::span<const core::Edge>(action_edges_.data() + action_first_edge_[idx], action_num_edges_[idx]);
  }
  std::shared_ptr<solver::Trainable> GetTrainable(uint32_t action_idx, uint64_t canonical_board_mask, size_t num_actions, size_t num_hands);
  void SetTrainable(uint32_t action_idx, uint64_t canonical_board_mask, std::shared_ptr<solver::Trainable> trainable);
  
  // Chance Nodes
  inline size_t GetNumChanceNodes() const { return chance_rounds_.size(); }
  inline core::GameRound GetChanceRound(uint32_t idx) const { return chance_rounds_[idx]; }
  inline double GetChancePot(uint32_t idx) const { return chance_pots_[idx]; }
  inline core::NodeRef GetChanceChild(uint32_t idx) const { return chance_children_[idx]; }
  inline std::span<const core::Card> GetChanceCards(uint32_t idx) const {
      return std::span<const core::Card>(chance_cards_.data() + chance_first_card_[idx], chance_num_cards_[idx]);
  }

  // Terminal Nodes
  inline size_t GetNumTerminalNodes() const { return terminal_pots_.size(); }
  inline double GetTerminalPot(uint32_t idx) const { return terminal_pots_[idx]; }
  inline std::span<const double> GetTerminalPayoffs(uint32_t idx) const {
      return std::span<const double>(terminal_payoffs_.data() + (idx * 2), 2);
  }

  // Showdown Nodes
  inline size_t GetNumShowdownNodes() const { return showdown_pots_.size(); }
  inline double GetShowdownPot(uint32_t idx) const { return showdown_pots_[idx]; }
  inline std::span<const double> GetShowdownPayoffs(uint32_t idx) const {
      return std::span<const double>(showdown_payoffs_.data() + (idx * 2), 2);
  }

private:
  // --- Dynamic Tree Building Helpers ---
  void BuildBranch(core::NodeRef current_node,
                   config::Rule current_rule,
                   const core::GameAction &last_action, int actions_this_round,
                   int raises_this_street);

  void BuildChanceNode(uint32_t node_idx, const config::Rule &rule);

  void BuildActionNode(uint32_t node_idx,
                       config::Rule current_rule_state,
                       const core::GameAction &last_action,
                       int actions_this_round, int raises_this_street);

  std::vector<double>
  GetPossibleBets(const config::Rule &rule, size_t player_index,
                  double current_player_commit, double opponent_commit,
                  double effective_stack, const core::GameAction &last_action,
                  double pot_before_action, core::GameRound round) const;

  static double RoundBet(double amount, double min_bet_increment);

  // --- SoA Node Storage ---
  
  // Action Nodes (SoA)
  alignas(64) std::vector<uint8_t> action_players_;
  alignas(64) std::vector<core::GameRound> action_rounds_;
  alignas(64) std::vector<double> action_pots_;
  alignas(64) std::vector<uint32_t> action_first_edge_;
  alignas(64) std::vector<uint16_t> action_num_edges_;
  alignas(64) std::vector<core::Edge> action_edges_;
  
  mutable std::mutex trainables_mutex_;
  std::vector<std::unordered_map<uint64_t, std::shared_ptr<solver::Trainable>>> action_trainables_; // By node, then by canonical board mask

  // Chance Nodes (SoA)
  alignas(64) std::vector<core::GameRound> chance_rounds_;
  alignas(64) std::vector<double> chance_pots_;
  alignas(64) std::vector<core::NodeRef> chance_children_;
  alignas(64) std::vector<uint32_t> chance_first_card_;
  alignas(64) std::vector<uint16_t> chance_num_cards_;
  alignas(64) std::vector<core::Card> chance_cards_;

  // Terminal Nodes (SoA)
  alignas(64) std::vector<double> terminal_pots_;
  alignas(64) std::vector<double> terminal_payoffs_; // Flattened 2D array [idx * 2 + player]

  // Showdown Nodes (SoA)
  alignas(64) std::vector<double> showdown_pots_;
  alignas(64) std::vector<double> showdown_payoffs_; // Flattened 2D array [idx * 2 + player]
  
  core::NodeRef root_ = core::kNullNode;
  core::Deck deck_;
  std::optional<config::Rule> build_rule_;

public:
  // --- Node Creation Helpers ---
  core::NodeRef CreateActionNode(uint8_t player, core::GameRound round, double pot);
  core::NodeRef CreateChanceNode(core::GameRound round, double pot, const std::vector<core::Card>& dealt_cards);
  core::NodeRef CreateTerminalNode(double pot, const std::vector<double>& payoffs);
  core::NodeRef CreateShowdownNode(core::GameRound round, double pot, const std::vector<double>& payoffs);
  void BuildActionNode(uint32_t node_idx, const std::vector<core::GameAction>& actions, const std::vector<core::NodeRef>& children);
  void SetRoot(core::NodeRef root) { root_ = root; }

private:
  // Deleted copy/move operations.
  GameTree(const GameTree &) = delete;
  GameTree &operator=(const GameTree &) = delete;
  GameTree(GameTree &&) = delete;
  GameTree &operator=(GameTree &&) = delete;
};

} // namespace core
} // namespace poker_solver

#endif // POKER_SOLVER_TREE_GAME_TREE_H_
