#ifndef POKER_SOLVER_SOLVER_DISCOUNTED_CFR_TRAINABLE_H_
#define POKER_SOLVER_SOLVER_DISCOUNTED_CFR_TRAINABLE_H_

#include "poker_solver/solver/information_set_strategy.h"
#include <cmath>
#include <nlohmann/json.hpp>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;

namespace poker_solver {
namespace solver {

/**
 * @brief CFR+ trainable node implementation.
 *
 * Implements Regret Matching+ (clamping negative regrets to 0) and
 * linear iteration-weighted averaging (weight t) across all hole-card combos.
 *
 * NOTE ON THREAD SAFETY:
 * Lazy evaluation caches (current_strategy_, average_strategy_) are written
 * via const methods GetCurrentStrategy() and GetAverageStrategy(). While safe
 * when each OpenMP thread works on disjoint nodes, concurrent reads to an
 * invalidated cache on the same node require external synchronization.
 */
class DiscountedCfrTrainable : public Trainable {
public:
  DiscountedCfrTrainable(size_t num_actions, size_t num_hands);
  ~DiscountedCfrTrainable() override = default;

  const std::vector<double>& GetCurrentStrategy() const override;
  const std::vector<double>& GetAverageStrategy() const override;

  void UpdateRegrets(const double* weighted_regrets, size_t size, int iteration) override;
  void UpdateRegrets(const std::vector<double>& weighted_regrets, int iteration) override {
    UpdateRegrets(weighted_regrets.data(), weighted_regrets.size(), iteration);
  }

  void AccumulateAverageStrategy(
      const std::vector<double>& current_strategy, const double* reach_probs, int iteration) override;

  void SetEv(const std::vector<double>& evs) override;

  json DumpStrategy(bool with_ev) const override;
  json DumpEvs() const override;

  void CopyStateFrom(const Trainable& other) override;

private:
  void CalculateCurrentStrategy() const;
  void CalculateAverageStrategy() const;

  size_t num_actions_;
  size_t num_hands_;
  std::vector<double> cumulative_regrets_;
  std::vector<double> cumulative_strategy_sum_;
  mutable std::vector<double> current_strategy_;
  mutable std::vector<double> average_strategy_;
  mutable bool current_strategy_valid_ = false;
  mutable bool average_strategy_valid_ = false;
  std::vector<double> expected_values_;
  bool has_evs_ = false;

  DiscountedCfrTrainable(const DiscountedCfrTrainable &) = delete;
  DiscountedCfrTrainable& operator=(const DiscountedCfrTrainable &) = delete;
  DiscountedCfrTrainable(DiscountedCfrTrainable &&) = delete;
  DiscountedCfrTrainable& operator=(DiscountedCfrTrainable &&) = delete;
};

} // namespace solver
} // namespace poker_solver

#endif // POKER_SOLVER_SOLVER_DISCOUNTED_CFR_TRAINABLE_H_
