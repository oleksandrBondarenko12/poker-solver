#include "utils/FlopIsomorphism.h"
#include "Card.h"
#include <algorithm>
#include <map>

namespace poker_solver {
namespace utils {

bool FlopIsomorphism::initialized_ = false;
std::vector<std::pair<uint64_t, double>> FlopIsomorphism::canonical_flops_;

void FlopIsomorphism::GetSuitMapping(uint64_t board_mask, int* out_mapping) {
    for (int i = 0; i < 4; ++i) out_mapping[i] = -1;
    int next_suit = 0;

    // Traverse cards in descending rank order
    for (int r = 12; r >= 0; --r) {
        for (int s = 0; s < 4; ++s) {
            int card_int = r * 4 + s;
            if ((board_mask & (1ULL << card_int)) != 0) {
                if (out_mapping[s] == -1) {
                    out_mapping[s] = next_suit++;
                }
            }
        }
    }
    
    // Fill remaining suits
    for (int i = 0; i < 4; ++i) {
        if (out_mapping[i] == -1) {
            out_mapping[i] = next_suit++;
        }
    }
}

std::array<int, 4> FlopIsomorphism::GetColorIsoOffset(uint64_t board_mask) {
    uint16_t color_hash[4] = {0, 0, 0, 0};
    uint64_t temp = board_mask;
    int c = 0;
    while (temp) {
        if (temp & 1) {
            int suitind = c % 4;
            int rankind = c / 4;
            color_hash[suitind] |= (1 << rankind);
        }
        temp >>= 1;
        c++;
    }
    
    std::array<int, 4> color_iso_offset = {0, 0, 0, 0};
    for (int i = 1; i < 4; i++) {
        for (int j = 0; j < i; j++) {
            if (color_hash[i] == color_hash[j]) {
                color_iso_offset[i] = j - i; 
                break;
            }
        }
    }
    return color_iso_offset;
}
uint64_t FlopIsomorphism::GetCanonicalBoard(uint64_t board_mask) {
    int suit_mapping[4];
    GetSuitMapping(board_mask, suit_mapping);

    uint64_t canonical_mask = 0;
    for (int r = 0; r < 13; ++r) {
        for (int s = 0; s < 4; ++s) {
            int card_int = r * 4 + s;
            if ((board_mask & (1ULL << card_int)) != 0) {
                int canon_suit = suit_mapping[s];
                int canon_card = r * 4 + canon_suit;
                canonical_mask |= (1ULL << canon_card);
            }
        }
    }
    return canonical_mask;
}

void FlopIsomorphism::Initialize() {
    if (initialized_) return;

    std::map<uint64_t, double> canon_map;

    // Generate all 22,100 flops
    for (int i = 0; i < 52; ++i) {
        for (int j = i + 1; j < 52; ++j) {
            for (int k = j + 1; k < 52; ++k) {
                uint64_t raw_mask = (1ULL << i) | (1ULL << j) | (1ULL << k);
                uint64_t canon_mask = GetCanonicalBoard(raw_mask);
                canon_map[canon_mask] += 1.0;
            }
        }
    }

    canonical_flops_.reserve(canon_map.size());
    for (const auto& pair : canon_map) {
        canonical_flops_.push_back(pair);
    }

    initialized_ = true;
}

const std::vector<std::pair<uint64_t, double>>& FlopIsomorphism::GetCanonicalFlops() {
    if (!initialized_) {
        Initialize();
    }
    return canonical_flops_;
}

} // namespace utils
} // namespace poker_solver
