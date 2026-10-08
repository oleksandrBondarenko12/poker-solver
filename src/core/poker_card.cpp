#include "poker_solver/core/poker_card.h"

#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <bit>

namespace poker_solver {
namespace core {

// --- Constants for Ranks and Suits ---
const std::array<char, kNumRanks> kRankChars = {
    '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
};
const std::array<char, kNumSuits> kSuitChars = {'c', 'd', 'h', 's'};

const std::array<char, kNumSuits>& Card::get_all_suit_chars() {
    return kSuitChars;
}
const std::array<char, kNumRanks>& Card::get_all_rank_chars() {
    return kRankChars;
}

// --- Helper Functions ---

bool Card::is_valid_card_int(int card_int) {
    return card_int >= 0 && card_int < kNumCardsInDeck;
}

char Card::suit_index_to_char(int suit_index) {
    if (suit_index >= 0 && suit_index < kNumSuits) {
        return kSuitChars[suit_index];
    }
    return '?';
}

char Card::rank_index_to_char(int rank_index) {
    if (rank_index >= 0 && rank_index < kNumRanks) {
        return kRankChars[rank_index];
    }
    return '?';
}

int Card::suit_char_to_index(char suit_char) {
    char lower_suit = std::tolower(suit_char);
    for (int i = 0; i < kNumSuits; ++i) {
        if (lower_suit == kSuitChars[i]) {
            return i;
        }
    }
    return -1;
}

int Card::rank_char_to_index(char rank_char) {
    char upper_rank = std::toupper(rank_char);
     for (int i = 0; i < kNumRanks; ++i) {
        if (upper_rank == kRankChars[i]) {
            return i;
        }
    }
    return -1;
}

// --- Constructors ---

Card::Card(int card_int) {
    if (is_valid_card_int(card_int)) {
        card_int_ = card_int;
    } else {
        std::ostringstream oss;
        oss << "Invalid card integer: " << card_int
            << ". Must be between 0 and " << kNumCardsInDeck - 1 << ".";
        throw std::out_of_range(oss.str());
    }
}

Card::Card(std::string_view card_str) {
    auto maybe_int = string_to_int(card_str);
    if (maybe_int) {
        card_int_ = *maybe_int;
    } else {
        std::ostringstream oss;
        oss << "Invalid card string: \"" << card_str
            << "\". Must be a 2-character string like 'As', 'Td', etc., or empty.";
        throw std::invalid_argument(oss.str());
    }
}

// --- Member Functions ---

std::string Card::to_string() const {
    if (!card_int_.has_value()) {
        return "Empty";
    }
    return int_to_string(*card_int_);
}

bool Card::operator==(const Card& other) const {
    return card_int_ == other.card_int_;
}

bool Card::operator!=(const Card& other) const {
    return !(*this == other);
}

// --- Static Conversion Utilities ---

std::optional<int> Card::string_to_int(std::string_view card_str) {
    if (card_str.empty()) {
        return std::nullopt;
    }
    if (card_str.length() != 2) {
        return std::nullopt;
    }

    int rank_index = rank_char_to_index(card_str[0]);
    int suit_index = suit_char_to_index(card_str[1]);

    if (rank_index == -1 || suit_index == -1) {
        return std::nullopt;
    }

    return rank_index * kNumSuits + suit_index;
}

std::string Card::int_to_string(int card_int) {
    if (!is_valid_card_int(card_int)) {
        return "Invalid";
    }
    int rank_index = card_int / kNumSuits;
    int suit_index = card_int % kNumSuits;

    std::string s;
    s += rank_index_to_char(rank_index);
    s += suit_index_to_char(suit_index);
    return s;
}

// --- Static Bitmask Utilities ---

uint64_t Card::card_int_to_uint64(int card_int) {
    if (!is_valid_card_int(card_int)) {
        throw std::out_of_range("Invalid card integer: " + std::to_string(card_int));
    }
    return 1ULL << card_int;
}

uint64_t Card::card_to_uint64(const Card& card) {
    if (card.is_empty()) {
        return 0ULL;
    }
    return card_int_to_uint64(*card.card_int());
}

uint64_t Card::card_ints_to_uint64(const std::vector<int>& card_ints) {
    uint64_t mask = 0ULL;
    for (int card_int : card_ints) {
        if (!is_valid_card_int(card_int)) {
             std::ostringstream oss;
             oss << "Invalid card integer encountered while creating mask: " << card_int;
             throw std::out_of_range(oss.str());
        }
        uint64_t card_bit = 1ULL << card_int;
        if (mask & card_bit) {
             std::ostringstream oss;
             oss << "Duplicate card integer encountered while creating mask: " << card_int;
             throw std::invalid_argument(oss.str());
        }
        mask |= card_bit;
    }
    return mask;
}

uint64_t Card::cards_to_uint64(const std::vector<Card>& cards) {
    uint64_t mask = 0ULL;
    for (const auto& card : cards) {
        if (!card.is_empty()) {
            mask |= card_to_uint64(card);
        }
    }
    return mask;
}

std::vector<int> Card::uint64_to_card_ints(uint64_t board_mask) {
    std::vector<int> card_ints;
    while (board_mask > 0) {
        int card_int = std::countr_zero(board_mask);
        card_ints.push_back(card_int);
        board_mask &= (board_mask - 1);
    }
    return card_ints;
}

std::vector<Card> Card::uint64_to_cards(uint64_t board_mask) {
    std::vector<int> card_ints = uint64_to_card_ints(board_mask);
    std::vector<Card> cards;
    cards.reserve(card_ints.size());
    for (int card_int : card_ints) {
        cards.emplace_back(card_int);
    }
    return cards;
}

bool Card::do_boards_overlap(uint64_t board_mask1, uint64_t board_mask2) {
    return (board_mask1 & board_mask2) != 0;
}

} // namespace core
} // namespace poker_solver
