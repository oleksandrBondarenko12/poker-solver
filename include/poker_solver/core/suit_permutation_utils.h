#ifndef POKER_SOLVER_CORE_SUIT_PERMUTATION_UTILS_H_
#define POKER_SOLVER_CORE_SUIT_PERMUTATION_UTILS_H_

#include "poker_solver/ranges/hole_card_combination.h" // For PrivateCards
#include "poker_solver/core/poker_card.h"          // For Card utilities
#include <vector>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <functional>

namespace poker_solver {
namespace utils {

// Exchanges elements in a value vector based on suit isomorphism.
template <typename T>
void exchange_suit_isomorphism(std::vector<T>& value,
                              const std::vector<core::PrivateHand>& range,
                              int suit_index1, int suit_index2) {

    if (value.size() != range.size()) {
        throw std::invalid_argument(
            "Value vector size must match range vector size in ExchangeSuitIsomorphism.");
    }
    if (suit_index1 < 0 || suit_index1 >= core::kNumSuits ||
        suit_index2 < 0 || suit_index2 >= core::kNumSuits) {
         throw std::out_of_range("Invalid suit index provided to ExchangeSuitIsomorphism.");
    }
    if (suit_index1 == suit_index2 || value.empty()) {
        return;
    }

    size_t range_size = range.size();
    // Map from the PrivateCards object itself back to its original index.
    std::unordered_map<core::PrivateHand, size_t, std::hash<core::PrivateHand>>
        hand_to_original_index;
    hand_to_original_index.reserve(range_size);

    for (size_t i = 0; i < range_size; ++i) {
         // Map the hand object to its first encountered index
         hand_to_original_index.try_emplace(range[i], i);
    }

    // Vector to track which indices have already been swapped.
    // Needs to be outside the loop to persist across iterations of i.
    std::vector<bool> swapped(range_size, false);

    for (size_t i = 0; i < range_size; ++i) {
        if (swapped[i]) {
            continue;
        }

        const core::PrivateHand& original_hand = range[i];
        int c1_orig = original_hand.Card1Int();
        int c2_orig = original_hand.Card2Int();

        // Create the isomorphic hand by swapping suits
        int c1_iso = c1_orig;
        int c2_iso = c2_orig;

        if (c1_orig % core::kNumSuits == suit_index1) c1_iso = c1_orig - suit_index1 + suit_index2;
        else if (c1_orig % core::kNumSuits == suit_index2) c1_iso = c1_orig - suit_index2 + suit_index1;

        if (c2_orig % core::kNumSuits == suit_index1) c2_iso = c2_orig - suit_index1 + suit_index2;
        else if (c2_orig % core::kNumSuits == suit_index2) c2_iso = c2_orig - suit_index2 + suit_index1;

        if (c1_iso == c1_orig && c2_iso == c2_orig) {
            swapped[i] = true; // Mark as processed even if no swap needed
            continue;
        }

        core::PrivateHand isomorphic_hand;
        try {
             isomorphic_hand = core::PrivateHand(c1_iso, c2_iso, original_hand.Weight());
        } catch (const std::invalid_argument& ) {
             swapped[i] = true;
             continue;
        }

        // Find the original index of this isomorphic hand using the object map
        auto it = hand_to_original_index.find(isomorphic_hand);

        if (it != hand_to_original_index.end()) {
            size_t j = it->second; // Index of the isomorphic partner hand

            if (i < j && !swapped[j]) {
                std::swap(value[i], value[j]);
                swapped[i] = true;
                swapped[j] = true;
            } else if (j < i && !swapped[j]) {
                swapped[i] = true;
            } else {
                 swapped[i] = true;
            }
        } else {
             swapped[i] = true;
        }
    }
}

template <typename T>
void ExchangeSuitIsomorphism(std::vector<T>& value,
                             const std::vector<core::PrivateHand>& range,
                             int suit_index1, int suit_index2) {
    exchange_suit_isomorphism(value, range, suit_index1, suit_index2);
}

} // namespace utils
} // namespace poker_solver

#endif // POKER_SOLVER_CORE_SUIT_PERMUTATION_UTILS_H_
