#ifndef POKER_SOLVER_SOLVER_DISCOUNTED_CFR_TRAINABLE_H_
#define POKER_SOLVER_SOLVER_DISCOUNTED_CFR_TRAINABLE_H_

#include "poker_solver/solver/information_set_strategy.h"
#include <cmath>
#include "poker_solver/json.hpp"
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;

namespace poker_solver {
namespace solver {

class DiscountedCfrTrainable : public Trainable {
public:
  DiscountedCfrTrainable(size_t num_actions, size_t num_hands);
  ~DiscountedCfrTrainable() override = default;

  const std::vector<double>& GetCurrentStrategy() const override;
  const std::vector<double>& GetAverageStrategy() const override;

  void UpdateRegrets(const std::vector<double>& weighted_regrets, int iteration) override;

  void AccumulateAverageStrategy(
      const std::vector<double>& current_strategy, const double* reach_probs, int iteration) override;

  void SetEv(const std::vector<double>& evs) override;

  json DumpStrategy(bool with_ev) const override;
  json DumpEvs() const override;

  void CopyStateFrom(const Trainable& other) override;

private:
  void CalculateCurrentStrategy();
  void CalculateAverageStrategy() const;

  static constexpr double kAlpha = 1.5;
  static constexpr double kBeta = 0.5;
  static constexpr double kGamma = 2.0;
  static constexpr double kTheta = 0.9;

  size_t num_actions_;
  size_t num_hands_;
  std::vector<double> cumulative_regrets_;
  std::vector<double> cumulative_strategy_sum_;
  mutable std::vector<double> current_strategy_;
  mutable std::vector<double> average_strategy_;
  mutable bool current_strategy_valid_ = false;
  mutable bool average_strategy_valid_ = false;
  std::vector<double> expected_values_;

  DiscountedCfrTrainable(const DiscountedCfrTrainable &) = delete;
  DiscountedCfrTrainable& operator=(const DiscountedCfrTrainable &) = delete;
  DiscountedCfrTrainable(DiscountedCfrTrainable &&) = delete;
  DiscountedCfrTrainable& operator=(DiscountedCfrTrainable &&) = delete;
};

} // namespace solver
} // namespace poker_solver

#endif // POKER_SOLVER_SOLVER_DISCOUNTED_CFR_TRAINABLE_H_
