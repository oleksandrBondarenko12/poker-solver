#ifndef POKER_SOLVER_CORE_POKER_CARD_H_
#define POKER_SOLVER_CORE_POKER_CARD_H_

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <stdexcept>
#include <sstream>

namespace poker_solver {
namespace core {

// Constants related to a standard 52-card deck.
constexpr int kNumCardsInDeck = 52;
constexpr int kNumSuits = 4;
constexpr int kNumRanks = 13;

// Represents a single playing card.
class Card {
 public:
  // --- Constructors ---
  Card() = default;
  explicit Card(int card_int);
  explicit Card(std::string_view card_str);

  // --- Accessors ---
  std::optional<int> card_int() const { return card_int_; }
  std::string to_string() const;
  bool is_empty() const { return !card_int_.has_value(); }

  // Backward-compatibility aliases
  std::string ToString() const { return to_string(); }
  bool IsEmpty() const { return is_empty(); }

  // --- Comparison Operators ---
  bool operator==(const Card& other) const;
  bool operator!=(const Card& other) const;

  // --- Static Conversion Utilities ---
  static std::optional<int> string_to_int(std::string_view card_str);
  static std::string int_to_string(int card_int);

  static std::optional<int> StringToInt(std::string_view card_str) { return string_to_int(card_str); }
  static std::string IntToString(int card_int) { return int_to_string(card_int); }

  // --- Static Bitmask Utilities ---
  static uint64_t card_ints_to_uint64(const std::vector<int>& card_ints);
  static uint64_t cards_to_uint64(const std::vector<Card>& cards);
  static uint64_t card_int_to_uint64(int card_int);
  static uint64_t card_to_uint64(const Card& card);
  static std::vector<int> uint64_to_card_ints(uint64_t board_mask);
  static std::vector<Card> uint64_to_cards(uint64_t board_mask);
  static bool do_boards_overlap(uint64_t board_mask1, uint64_t board_mask2);

  static uint64_t CardIntsToUint64(const std::vector<int>& card_ints) { return card_ints_to_uint64(card_ints); }
  static uint64_t CardsToUint64(const std::vector<Card>& cards) { return cards_to_uint64(cards); }
  static uint64_t CardIntToUint64(int card_int) { return card_int_to_uint64(card_int); }
  static uint64_t CardToUint64(const Card& card) { return card_to_uint64(card); }
  static std::vector<int> Uint64ToCardInts(uint64_t board_mask) { return uint64_to_card_ints(board_mask); }
  static std::vector<Card> Uint64ToCards(uint64_t board_mask) { return uint64_to_cards(board_mask); }
  static bool DoBoardsOverlap(uint64_t board_mask1, uint64_t board_mask2) { return do_boards_overlap(board_mask1, board_mask2); }

  // --- Static Rank/Suit Helpers ---
  static char suit_index_to_char(int suit_index);
  static char rank_index_to_char(int rank_index);
  static int suit_char_to_index(char suit_char);
  static int rank_char_to_index(char rank_char);
  static const std::array<char, kNumSuits>& get_all_suit_chars();
  static const std::array<char, kNumRanks>& get_all_rank_chars();
  static bool is_valid_card_int(int card_int);

  static char SuitIndexToChar(int suit_index) { return suit_index_to_char(suit_index); }
  static char RankIndexToChar(int rank_index) { return rank_index_to_char(rank_index); }
  static int SuitCharToIndex(char suit_char) { return suit_char_to_index(suit_char); }
  static int RankCharToIndex(char rank_char) { return rank_char_to_index(rank_char); }
  static const std::array<char, kNumSuits>& GetAllSuitChars() { return get_all_suit_chars(); }
  static const std::array<char, kNumRanks>& GetAllRankChars() { return get_all_rank_chars(); }
  static bool IsValidCardInt(int card_int) { return is_valid_card_int(card_int); }

 private:
  std::optional<int> card_int_ = std::nullopt;
};

using PokerCard = Card;

} // namespace core
} // namespace poker_solver

#endif // POKER_SOLVER_CORE_POKER_CARD_H_
