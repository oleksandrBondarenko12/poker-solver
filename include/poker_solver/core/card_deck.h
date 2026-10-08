#ifndef POKER_SOLVER_CORE_CARD_DECK_H_
#define POKER_SOLVER_CORE_CARD_DECK_H_

#include "poker_solver/core/poker_card.h"
#include <vector>
#include <string>
#include <string_view>

namespace poker_solver {
namespace core {

// Represents a standard 52-card deck.
class Deck {
 public:
  // Creates a standard, ordered 52-card deck.
  Deck();

  // Creates a deck using custom ranks and suits.
  Deck(const std::vector<std::string_view>& ranks,
       const std::vector<std::string_view>& suits);

  // Returns a const reference to the vector of cards in the deck.
  const std::vector<Card>& get_cards() const;
  const std::vector<Card>& GetCards() const { return get_cards(); }

  // Finds a card by its string representation.
  Card find_card(std::string_view card_str) const;
  Card FindCard(std::string_view card_str) const { return find_card(card_str); }

  // Finds a card by its integer representation (0-51).
  Card find_card(int card_int) const;
  Card FindCard(int card_int) const { return find_card(card_int); }

 private:
  std::vector<Card> cards_;
};

using CardDeck = Deck;

} // namespace core
} // namespace poker_solver

#endif // POKER_SOLVER_CORE_CARD_DECK_H_
