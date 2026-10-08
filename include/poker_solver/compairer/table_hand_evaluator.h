#ifndef POKER_SOLVER_EVAL_TABLE_HAND_EVALUATOR_H_
#define POKER_SOLVER_EVAL_TABLE_HAND_EVALUATOR_H_

#include "poker_solver/core/poker_card.h"
#include "poker_solver/core/solver_math_utils.h"
#include "poker_solver/compairer/hand_strength_evaluator.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace poker_solver {
namespace eval {

// Concrete implementation of Compairer using a pre-computed dictionary
// of 5-card hand ranks loaded from a file (with binary caching).
class Dic5Compairer : public core::Compairer {
 public:
  static constexpr int kInvalidRank = 999999;

  // Static Bitmask Helpers
  static uint64_t ranks_hash(uint64_t cards_mask);
  static bool is_flush(uint64_t cards_mask);

  static uint64_t RanksHash(uint64_t cards_mask) { return ranks_hash(cards_mask); }
  static bool IsFlush(uint64_t cards_mask) { return is_flush(cards_mask); }

  explicit Dic5Compairer(const std::string& dictionary_filepath);

  // Modern snake_case interface
  core::ComparisonResult compare_hands(const std::vector<int>& private_hand1,
                                     const std::vector<int>& private_hand2,
                                     const std::vector<int>& public_board) const override;

  core::ComparisonResult compare_hands(uint64_t private_mask1,
                                     uint64_t private_mask2,
                                     uint64_t public_mask) const override;

  int get_hand_rank(const std::vector<int>& private_hand,
                    const std::vector<int>& public_board) const override;

  int get_hand_rank(uint64_t private_mask, uint64_t public_mask) const override;

  int get_best_rank_for_cards(const std::vector<int>& cards) const;
  int GetBestRankForCards(const std::vector<int>& cards) const { return get_best_rank_for_cards(cards); }

 private:
  void load_dictionary_from_text(const std::string& filepath);
  bool load_binary_cache(const std::filesystem::path& cache_filepath);
  bool save_binary_cache(const std::filesystem::path& cache_filepath) const;
  int lookup_5_card_rank(uint64_t hand_mask) const;

  std::unordered_map<uint64_t, int> flush_ranks_;
  std::unordered_map<uint64_t, int> non_flush_ranks_;
  std::filesystem::path dictionary_path_;
  std::filesystem::path cache_path_;
};

using TableHandEvaluator = Dic5Compairer;

} // namespace eval
} // namespace poker_solver

#endif // POKER_SOLVER_EVAL_TABLE_HAND_EVALUATOR_H_
