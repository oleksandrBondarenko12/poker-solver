#ifndef POKER_SOLVER_CORE_GAME_TREE_NODE_H_
#define POKER_SOLVER_CORE_GAME_TREE_NODE_H_

#include <cstdint>
#include <string>
#include <stdexcept>

namespace poker_solver {
namespace core {

// Enum representing the different betting rounds in Hold'em.
enum class GameRound { kPreflop = 0, kFlop = 1, kTurn = 2, kRiver = 3 };

// Enum representing the possible types of nodes in the game tree.
enum class GameTreeNodeType : uint8_t { kAction = 0, kChance = 1, kShowdown = 2, kTerminal = 3 };

// Enum representing the possible actions a player can take, plus game states.
enum class PokerAction {
  kBegin,      // Represents the absolute start of the game/tree
  kRoundBegin, // Represents the start of a betting round
  kBet,
  kRaise,
  kCheck,
  kFold,
  kCall
};

// Flat index referencing a specific node inside GameTree's SoA arrays.
struct NodeRef {
    GameTreeNodeType type;
    uint32_t index;

    // Helper for debugging/comparisons
    bool operator==(const NodeRef& other) const {
        return type == other.type && index == other.index;
    }
};
constexpr NodeRef kNullNode = {GameTreeNodeType::kTerminal, 0xFFFFFFFF};

class GameTreeNode {
public:
  // Static Helpers
  static GameRound IntToGameRound(int round_int) {
    if (round_int < 0 || round_int > 3) throw std::out_of_range("Invalid round int");
    return static_cast<GameRound>(round_int);
  }
  static int GameRoundToInt(GameRound game_round) {
    return static_cast<int>(game_round);
  }
  static std::string GameRoundToString(GameRound game_round) {
    switch (game_round) {
      case GameRound::kPreflop: return "Preflop";
      case GameRound::kFlop:    return "Flop";
      case GameRound::kTurn:    return "Turn";
      case GameRound::kRiver:   return "River";
      default: return "Unknown";
    }
  }
};

} // namespace core
} // namespace poker_solver

#endif // POKER_SOLVER_CORE_GAME_TREE_NODE_H_
