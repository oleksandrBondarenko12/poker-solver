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
    static std::array<int, 4> get_color_iso_offset(uint64_t board_mask);

    // Backward-compatibility wrappers
    static const std::vector<std::pair<uint64_t, double>>& GetCanonicalFlops() { return get_canonical_flops(); }
    static uint64_t GetCanonicalBoard(uint64_t board_mask) { return get_canonical_board(board_mask); }
    static void GetSuitMapping(uint64_t board_mask, int* out_mapping) { get_suit_mapping(board_mask, out_mapping); }
    static std::array<int, 4> GetColorIsoOffset(uint64_t board_mask) { return get_color_iso_offset(board_mask); }

private:
    static void initialize();
    static bool initialized_;
    static std::vector<std::pair<uint64_t, double>> canonical_flops_;
};

using CanonicalFlopIsomorphism = FlopIsomorphism;

} // namespace utils
} // namespace poker_solver

#endif // POKER_SOLVER_UTILS_FLOP_ISOMORPHISM_H_
