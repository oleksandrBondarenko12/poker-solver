#ifndef POKER_SOLVER_UTILS_FLOP_ISOMORPHISM_H_
#define POKER_SOLVER_UTILS_FLOP_ISOMORPHISM_H_

#include <vector>
#include <cstdint>
#include <utility>
#include <array>

namespace poker_solver {
namespace utils {

class FlopIsomorphism {
public:
    // Retrieves the 1,755 canonical flop masks and their corresponding probability weights (e.g., 4, 12, 24).
    // The weights sum to exactly 22,100.
    static const std::vector<std::pair<uint64_t, double>>& GetCanonicalFlops();

    // Returns the canonical board mask for any given board mask (3, 4, or 5 cards).
    // This maps suits such that the first seen suit becomes Spades (0), second becomes Hearts (1), etc.
    // The cards are evaluated in descending rank order.
    static uint64_t GetCanonicalBoard(uint64_t board_mask);

    // Returns a suit mapping array of size 4, where mapping[original_suit] = canonical_suit.
    // If a suit is not present in the board, it is assigned the next available canonical suit.
    static void GetSuitMapping(uint64_t board_mask, int* out_mapping);

    // Evaluates a board mask and returns an array of color offsets for single-card isomorphism.
    // Specifically, for each suit i in 0..3:
    // If suit i is isomorphic to an earlier suit j (where j < i), offset[i] = (i - j).
    // If suit i is not isomorphic to any earlier suit, offset[i] = 0.
    static std::array<int, 4> GetColorIsoOffset(uint64_t board_mask);

private:
    static void Initialize();
    static bool initialized_;
    static std::vector<std::pair<uint64_t, double>> canonical_flops_;
};

} // namespace utils
} // namespace poker_solver

#endif // POKER_SOLVER_UTILS_FLOP_ISOMORPHISM_H_
