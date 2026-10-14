#include "poker_solver/core/canonical_flop_isomorphism.h"
#include "poker_solver/core/poker_card.h"
#include <algorithm>
#include <map>

namespace poker_solver {
namespace utils {

void FlopIsomorphism::get_suit_mapping(uint64_t board_mask, int* out_mapping) {
    auto card_ints = core::Card::Uint64ToCardInts(board_mask & 0x000FFFFFFFFFFFFFULL);
    if (card_ints.size() != 3) {
        for (int i = 0; i < 4; ++i) out_mapping[i] = i;
        return;
    }
    std::array<int, 3> best_tuple = {-1, -1, -1};
    std::array<int, 4> best_perm = {0, 1, 2, 3};

    std::array<int, 4> p = {0, 1, 2, 3};
    do {
        std::array<int, 3> mapped;
        for (size_t i = 0; i < 3; ++i) {
            int rank = card_ints[i] / 4;
            int suit = card_ints[i] % 4;
            mapped[i] = rank * 4 + p[suit];
        }
        std::sort(mapped.begin(), mapped.end(), std::greater<int>());
        if (mapped > best_tuple) {
            best_tuple = mapped;
            best_perm = p;
        }
    } while (std::next_permutation(p.begin(), p.end()));

    for (int i = 0; i < 4; ++i) {
        out_mapping[i] = best_perm[i];
    }
}

std::array<int, 4> FlopIsomorphism::get_suit_iso_offset(uint64_t board_mask) {
    uint16_t suit_hash[4] = {0, 0, 0, 0};
    uint64_t temp = board_mask & 0x000FFFFFFFFFFFFFULL;
    int c = 0;
    while (temp) {
        if (temp & 1) {
            int suitind = c % 4;
            int rankind = c / 4;
            suit_hash[suitind] |= (1 << rankind);
        }
        temp >>= 1;
        c++;
    }
    
    std::array<int, 4> suit_iso_offset = {0, 0, 0, 0};
    for (int i = 1; i < 4; i++) {
        for (int j = 0; j < i; j++) {
            if (suit_hash[i] == suit_hash[j]) {
                suit_iso_offset[i] = j - i; 
                break;
            }
        }
    }
    return suit_iso_offset;
}

uint64_t FlopIsomorphism::get_canonical_board(uint64_t board_mask) {
    uint64_t clean_mask = board_mask & 0x000FFFFFFFFFFFFFULL;
    auto card_ints = core::Card::Uint64ToCardInts(clean_mask);
    if (card_ints.size() != 3) {
        return clean_mask;
    }
    std::array<int, 3> best_tuple = {-1, -1, -1};
    uint64_t best_mask = board_mask;

    std::array<int, 4> p = {0, 1, 2, 3};
    do {
        std::array<int, 3> mapped;
        for (size_t i = 0; i < 3; ++i) {
            int rank = card_ints[i] / 4;
            int suit = card_ints[i] % 4;
            mapped[i] = rank * 4 + p[suit];
        }
        std::sort(mapped.begin(), mapped.end(), std::greater<int>());
        if (mapped > best_tuple) {
            best_tuple = mapped;
            best_mask = (1ULL << mapped[0]) | (1ULL << mapped[1]) | (1ULL << mapped[2]);
        }
    } while (std::next_permutation(p.begin(), p.end()));

    return best_mask;
}

bool FlopIsomorphism::initialized_ = false;
std::vector<std::pair<uint64_t, double>> FlopIsomorphism::canonical_flops_;
std::vector<std::array<int, 24>> FlopIsomorphism::canonical_flop_perm_counts_;
std::vector<std::array<int, 4>> FlopIsomorphism::all_permutations_;

void FlopIsomorphism::initialize() {
    if (initialized_) return;

    all_permutations_.clear();
    std::array<int, 4> base_p = {0, 1, 2, 3};
    do {
        all_permutations_.push_back(base_p);
    } while (std::next_permutation(base_p.begin(), base_p.end()));

    std::array<int, 256> code_to_idx;
    code_to_idx.fill(-1);
    for (size_t p = 0; p < all_permutations_.size(); ++p) {
        const auto& perm = all_permutations_[p];
        int code = (perm[0] << 6) | (perm[1] << 4) | (perm[2] << 2) | perm[3];
        code_to_idx[code] = static_cast<int>(p);
    }

    struct CanonData {
        double weight = 0.0;
        std::array<int, 24> perm_counts = {0};
    };
    std::map<uint64_t, CanonData> canon_map;

    // Generate all 22,100 flops
    for (int i = 0; i < 52; ++i) {
        for (int j = i + 1; j < 52; ++j) {
            for (int k = j + 1; k < 52; ++k) {
                uint64_t raw_mask = (1ULL << i) | (1ULL << j) | (1ULL << k);
                uint64_t canon_mask = get_canonical_board(raw_mask);
                int suit_mapping[4];
                get_suit_mapping(raw_mask, suit_mapping);
                int code = (suit_mapping[0] << 6) | (suit_mapping[1] << 4) | (suit_mapping[2] << 2) | suit_mapping[3];
                int perm_idx = code_to_idx[code];

                auto& entry = canon_map[canon_mask];
                entry.weight += 1.0;
                if (perm_idx >= 0 && perm_idx < 24) {
                    entry.perm_counts[perm_idx]++;
                }
            }
        }
    }

    canonical_flops_.clear();
    canonical_flop_perm_counts_.clear();
    canonical_flops_.reserve(canon_map.size());
    canonical_flop_perm_counts_.reserve(canon_map.size());
    for (const auto& pair : canon_map) {
        canonical_flops_.push_back({pair.first, pair.second.weight});
        canonical_flop_perm_counts_.push_back(pair.second.perm_counts);
    }

    initialized_ = true;
}

const std::vector<std::pair<uint64_t, double>>& FlopIsomorphism::get_canonical_flops() {
    if (!initialized_) {
        initialize();
    }
    return canonical_flops_;
}

const std::vector<std::array<int, 24>>& FlopIsomorphism::get_canonical_flop_perm_counts() {
    if (!initialized_) {
        initialize();
    }
    return canonical_flop_perm_counts_;
}

const std::array<int, 4>& FlopIsomorphism::get_permutation_by_index(int perm_idx) {
    if (!initialized_) {
        initialize();
    }
    static const std::array<int, 4> kDefault = {0, 1, 2, 3};
    if (perm_idx < 0 || perm_idx >= static_cast<int>(all_permutations_.size())) {
        return kDefault;
    }
    return all_permutations_[perm_idx];
}

} // namespace utils
} // namespace poker_solver
