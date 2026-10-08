#ifndef POKER_SOLVER_CORE_HOLE_CARD_COMBINATION_H_
#define POKER_SOLVER_CORE_HOLE_CARD_COMBINATION_H_

#include "poker_solver/core/poker_card.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <utility>

namespace poker_solver {
namespace core {

// Represents a player's two private hole cards, along with an associated weight.
class PrivateCards {
 public:
  PrivateCards();
  PrivateCards(int card1_int, int card2_int, double weight = 1.0);

  // Modern snake_case accessors
  int card1_int() const { return card1_int_; }
  int card2_int() const { return card2_int_; }
  double weight() const { return weight_; }
  void set_weight(double w) { weight_ = w; }
  double relative_prob() const { return relative_prob_; }
  void set_relative_prob(double p) { relative_prob_ = p; }
  uint64_t board_mask() const { return board_mask_; }
  std::string to_string() const;
  std::pair<int, int> card_ints() const { return {card1_int_, card2_int_}; }

  // Backward-compatibility aliases
  int Card1Int() const { return card1_int(); }
  int Card2Int() const { return card2_int(); }
  double Weight() const { return weight(); }
  void SetWeight(double w) { set_weight(w); }
  double GetRelativeProb() const { return relative_prob(); }
  void SetRelativeProb(double p) { set_relative_prob(p); }
  uint64_t GetBoardMask() const { return board_mask(); }
  std::string ToString() const { return to_string(); }
  std::pair<int, int> GetCardInts() const { return card_ints(); }

  // Operators
  bool operator==(const PrivateCards& other) const;
  bool operator!=(const PrivateCards& other) const;
  bool operator<(const PrivateCards& other) const;

 private:
  void initialize(int c1, int c2, double w);

  int card1_int_;
  int card2_int_;
  double weight_;
  double relative_prob_;
  uint64_t board_mask_;
};

using HoleCardCombination = PrivateCards;

} // namespace core
} // namespace poker_solver

namespace std {
template <>
struct hash<poker_solver::core::PrivateCards> {
  size_t operator()(const poker_solver::core::PrivateCards& pc) const {
    return std::hash<uint64_t>()(pc.board_mask());
  }
};
} // namespace std

#endif // POKER_SOLVER_CORE_HOLE_CARD_COMBINATION_H_
