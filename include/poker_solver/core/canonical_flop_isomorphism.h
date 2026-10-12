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
    // Modern snake_case interface
    static const std::vector<std::pair<uint64_t, double>>& get_canonical_flops();
    static uint64_t get_canonical_board(uint64_t board_mask);
    static void get_suit_mapping(uint64_t board_mask, int* out_mapping);
    static std::array<int, 4> get_suit_iso_offset(uint64_t board_mask);

    static const std::vector<std::array<int, 24>>& get_canonical_flop_perm_counts();
    static const std::array<int, 4>& get_permutation_by_index(int perm_idx);

    // Convenience wrappers
    static const std::vector<std::pair<uint64_t, double>>& GetCanonicalFlops() { return get_canonical_flops(); }
    static const std::vector<std::array<int, 24>>& GetCanonicalFlopPermCounts() { return get_canonical_flop_perm_counts(); }
    static const std::array<int, 4>& GetPermutationByIndex(int perm_idx) { return get_permutation_by_index(perm_idx); }
    static uint64_t GetCanonicalBoard(uint64_t board_mask) { return get_canonical_board(board_mask); }
    static void GetSuitMapping(uint64_t board_mask, int* out_mapping) { get_suit_mapping(board_mask, out_mapping); }
    static std::array<int, 4> GetSuitIsoOffset(uint64_t board_mask) { return get_suit_iso_offset(board_mask); }

private:
    static void initialize();
    static bool initialized_;
    static std::vector<std::pair<uint64_t, double>> canonical_flops_;
    static std::vector<std::array<int, 24>> canonical_flop_perm_counts_;
    static std::vector<std::array<int, 4>> all_permutations_;
};

using CanonicalFlopIsomorphism = FlopIsomorphism;

} // namespace utils
} // namespace poker_solver

#endif // POKER_SOLVER_UTILS_FLOP_ISOMORPHISM_H_
