#ifndef POKER_SOLVER_SOLVER_TRAINABLE_H_
#define POKER_SOLVER_SOLVER_TRAINABLE_H_

#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace poker_solver {
namespace solver {

class Trainable {
public:
  virtual ~Trainable() = default;

  virtual const std::vector<double> &GetCurrentStrategy() const = 0;
  virtual const std::vector<double> &GetAverageStrategy() const = 0;

  // Regrets arrive pre-weighted by opp_reach * chance_reach.
  // Implementations accumulate incoming weighted regret (e.g. CFR+ non-negative
  // regret matching).
  virtual void UpdateRegrets(const std::vector<double> &weighted_regrets,
                             int iteration) = 0;

  // Accumulates the average strategy.
  // Implementations use the per-hand reach weights provided in reach_probs and
  // iteration weighting (e.g. linear t for CFR+).
  virtual void
  AccumulateAverageStrategy(const std::vector<double> &current_strategy,
                            const double *reach_probs, int iteration) = 0;

  virtual void SetEv(const std::vector<double> &evs) = 0;
  virtual json DumpStrategy(bool with_ev) const = 0;
  virtual json DumpEvs() const = 0;
  virtual void CopyStateFrom(const Trainable &other) = 0;

protected:
  Trainable() = default;
  Trainable(const Trainable &) = delete;
  Trainable &operator=(const Trainable &) = delete;
  Trainable(Trainable &&) = delete;
  Trainable &operator=(Trainable &&) = delete;
};
} // namespace solver
} // namespace poker_solver

#endif // POKER_SOLVER_SOLVER_TRAINABLE_H_
