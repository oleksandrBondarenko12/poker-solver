#ifndef POKER_SOLVER_RANGES_RANGE_SYNTAX_PARSER_H_
#define POKER_SOLVER_RANGES_RANGE_SYNTAX_PARSER_H_

#include "poker_solver/ranges/hole_card_combination.h"
#include "poker_solver/core/poker_card.h"
#include <vector>
#include <string>
#include <string_view>
#include <stdexcept>

namespace poker_solver {
namespace ranges {

// Converts poker hand range strings into collections of PrivateCards objects.
class PrivateRangeConverter {
 public:
  static std::vector<core::PrivateCards> string_to_private_cards(
      std::string_view range_string,
      const std::vector<int>& initial_board_ints = {});

  static std::vector<core::PrivateCards> StringToPrivateCards(
      std::string_view range_string,
      const std::vector<int>& initial_board_ints = {}) {
    return string_to_private_cards(range_string, initial_board_ints);
  }

 private:
  PrivateRangeConverter() = delete;

  static void parse_range_component(
      std::string_view component,
      uint64_t initial_board_mask,
      std::vector<core::PrivateCards>& output_cards);

  static void generate_pair_combos(
      char rank_char,
      double weight,
      uint64_t initial_board_mask,
      std::vector<core::PrivateCards>& output_cards);

  static void generate_suited_combos(
      char rank1_char,
      char rank2_char,
      double weight,
      uint64_t initial_board_mask,
      std::vector<core::PrivateCards>& output_cards);

  static void generate_offsuit_combos(
      char rank1_char,
      char rank2_char,
      double weight,
      uint64_t initial_board_mask,
      std::vector<core::PrivateCards>& output_cards);

  static void generate_specific_combo(
      std::string_view combo_str,
      double weight,
      uint64_t initial_board_mask,
      std::vector<core::PrivateCards>& output_cards);
};

using RangeSyntaxParser = PrivateRangeConverter;

} // namespace ranges
} // namespace poker_solver

#endif // POKER_SOLVER_RANGES_RANGE_SYNTAX_PARSER_H_
