#include "poker_solver/core/card_deck.h"

#include <stdexcept>
#include <sstream>

namespace poker_solver {
namespace core {

// Constructor for a standard 52-card deck.
Deck::Deck() {
    cards_.reserve(kNumCardsInDeck);
    for (int i = 0; i < kNumCardsInDeck; ++i) {
        cards_.emplace_back(i);
    }
}

// Constructor for a custom deck.
Deck::Deck(const std::vector<std::string_view>& ranks,
           const std::vector<std::string_view>& suits) {
    cards_.reserve(ranks.size() * suits.size());
    for (const auto& rank : ranks) {
        for (const auto& suit : suits) {
            std::string card_str;
            card_str.append(rank);
            card_str.append(suit);
            try {
                cards_.emplace_back(card_str);
            } catch (const std::invalid_argument& e) {
                std::ostringstream oss;
                oss << "Error creating custom deck with rank '" << rank
                    << "' and suit '" << suit << "': " << e.what();
                throw std::runtime_error(oss.str());
            }
        }
    }
}

const std::vector<Card>& Deck::get_cards() const {
    return cards_;
}

Card Deck::find_card(std::string_view card_str) const {
    std::optional<int> target_int = Card::string_to_int(card_str);
    if (!target_int) {
        return Card();
    }
    return find_card(target_int.value());
}

Card Deck::find_card(int card_int) const {
    if (!Card::is_valid_card_int(card_int)) {
         return Card();
    }
    if (cards_.size() == kNumCardsInDeck) {
         if (static_cast<size_t>(card_int) < cards_.size() &&
             cards_[card_int].card_int() == card_int) {
             return cards_[card_int];
         }
    }
    for (const auto& card : cards_) {
        if (card.card_int() == card_int) {
            return card;
        }
    }
    return Card();
}

} // namespace core
} // namespace poker_solver
