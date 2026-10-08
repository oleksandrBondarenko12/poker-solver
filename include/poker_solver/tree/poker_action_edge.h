#ifndef POKER_SOLVER_CORE_GAME_ACTIONS_H_
#define POKER_SOLVER_CORE_GAME_ACTIONS_H_

#include "poker_solver/tree/game_tree_node_types.h" // For PokerAction enum (adjust path if needed)
#include <string>
#include <stdexcept> // For exceptions
#include <sstream>   // For error messages

namespace poker_solver {
namespace core {

// Represents a specific action taken in a poker game (e.g., BET, CHECK, FOLD)
// along with an associated amount, if applicable.
class GameAction {
 public:
  // Default constructor (creates an invalid BEGIN action).
  GameAction();

  // Constructs a game action.
  // Args:
  //   action: The type of poker action (e.g., PokerAction::kBet).
  //   amount: The amount associated with the action (required for BET/RAISE,
  //           should be -1 or ignored otherwise). Defaults to -1.0.
  // Throws:
  //   std::invalid_argument if amount rules are violated (e.g., amount provided
  //                         for CHECK, or no amount for BET/RAISE).
  explicit GameAction(PokerAction action, double amount = -1.0);

  // Modern snake_case accessors
  PokerAction action() const { return action_; }
  double amount() const { return amount_; }
  std::string to_string() const;
  static std::string action_to_string(PokerAction action);

  // Backward-compatibility wrappers
  PokerAction GetAction() const { return action(); }
  double GetAmount() const { return amount(); }
  std::string ToString() const { return to_string(); }
  static std::string ActionToString(PokerAction act) { return action_to_string(act); }

 private:
  PokerAction action_;
  double amount_; // Amount associated with BET/RAISE actions. -1 otherwise.
};

using PokerActionEdge = GameAction;

} // namespace core
} // namespace poker_solver

#endif // POKER_SOLVER_CORE_GAME_ACTIONS_H_
