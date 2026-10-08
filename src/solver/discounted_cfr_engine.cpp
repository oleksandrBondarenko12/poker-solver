#include "poker_solver/solver/discounted_cfr_engine.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include "poker_solver/json.hpp"
#include <limits>
#include <numeric>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <vector>

using json = nlohmann::json;

namespace poker_solver {
namespace solver {

DiscountedCfrTrainable::DiscountedCfrTrainable(size_t num_actions, size_t num_hands)
    : num_actions_(num_actions), num_hands_(num_hands) {

  size_t total_size = num_actions_ * num_hands_;
  if (total_size > 0) {
    cumulative_regrets_.resize(total_size, 0.0);
    cumulative_strategy_sum_.resize(total_size, 0.0);

    double initial_prob = (num_actions_ > 0) ? 1.0 / static_cast<double>(num_actions_) : 0.0;
    current_strategy_.resize(total_size);
    average_strategy_.resize(total_size);
    for (size_t h = 0; h < num_hands_; ++h) {
      for (size_t a = 0; a < num_actions_; ++a) {
        size_t index = h * num_actions_ + a;
        current_strategy_[index] = initial_prob;
        average_strategy_[index] = initial_prob;
      }
    }
    expected_values_.resize(total_size, std::numeric_limits<double>::quiet_NaN());

    current_strategy_valid_ = true;
    average_strategy_valid_ = true;
  } else {
    current_strategy_valid_ = true;
    average_strategy_valid_ = true;
  }
}

void DiscountedCfrTrainable::CalculateCurrentStrategy() {
  if (current_strategy_valid_) return;

  if (num_actions_ == 0 || num_hands_ == 0) {
    current_strategy_.clear();
    current_strategy_valid_ = true;
    return;
  }

  double default_prob = (num_actions_ > 0) ? 1.0 / static_cast<double>(num_actions_) : 0.0;
  size_t total_size = num_actions_ * num_hands_;
  if (current_strategy_.size() != total_size) current_strategy_.resize(total_size);

  for (size_t h = 0; h < num_hands_; ++h) {
    double regret_sum = 0.0;
    for (size_t a = 0; a < num_actions_; ++a) {
      size_t index = h * num_actions_ + a;
      regret_sum += std::max(0.0, cumulative_regrets_[index]);
    }

    for (size_t a = 0; a < num_actions_; ++a) {
      size_t index = h * num_actions_ + a;
      if (regret_sum > 1e-12) {
        current_strategy_[index] = std::max(0.0, cumulative_regrets_[index]) / regret_sum;
      } else {
        current_strategy_[index] = default_prob;
      }
    }
  }
  current_strategy_valid_ = true;
}

void DiscountedCfrTrainable::CalculateAverageStrategy() const {
  if (average_strategy_valid_) return;

  if (num_actions_ == 0 || num_hands_ == 0) {
    average_strategy_.clear();
    average_strategy_valid_ = true;
    return;
  }

  double default_prob = (num_actions_ > 0) ? 1.0 / static_cast<double>(num_actions_) : 0.0;
  size_t total_size = num_actions_ * num_hands_;
  if (average_strategy_.size() != total_size) average_strategy_.resize(total_size);

  for (size_t h = 0; h < num_hands_; ++h) {
    double strategy_sum_total = 0.0;
    for (size_t a = 0; a < num_actions_; ++a) {
      size_t index = h * num_actions_ + a;
      strategy_sum_total += cumulative_strategy_sum_[index];
    }

    for (size_t a = 0; a < num_actions_; ++a) {
      size_t index = h * num_actions_ + a;
      if (strategy_sum_total > 1e-12) {
        average_strategy_[index] = cumulative_strategy_sum_[index] / strategy_sum_total;
      } else {
        average_strategy_[index] = default_prob;
      }
    }
  }
  average_strategy_valid_ = true;
}

const std::vector<double>& DiscountedCfrTrainable::GetCurrentStrategy() const {
  if (!current_strategy_valid_) {
    const_cast<DiscountedCfrTrainable *>(this)->CalculateCurrentStrategy();
  }
  return current_strategy_;
}

const std::vector<double>& DiscountedCfrTrainable::GetAverageStrategy() const {
  if (!average_strategy_valid_) {
    CalculateAverageStrategy();
  }
  return average_strategy_;
}

void DiscountedCfrTrainable::UpdateRegrets(const std::vector<double>& weighted_regrets, int iteration) {
  size_t total_size = num_actions_ * num_hands_;
  if (weighted_regrets.size() != total_size) {
    throw std::invalid_argument("Regret vector size mismatch in UpdateRegrets.");
  }
  if (iteration <= 0) {
    throw std::invalid_argument("Iteration number must be positive in UpdateRegrets.");
  }

  for (size_t i = 0; i < total_size; ++i) {
    cumulative_regrets_[i] += weighted_regrets[i];
    
    // CFR+ negative regret clamping
    if (cumulative_regrets_[i] < 0.0) {
      cumulative_regrets_[i] = 0.0;
    }
  }

  current_strategy_valid_ = false;
  average_strategy_valid_ = false;
}

void DiscountedCfrTrainable::AccumulateAverageStrategy(
    const std::vector<double>& current_strategy, const double* reach_probs, int iteration) {
  // Linear weighting for CFR+
  double current_weight = static_cast<double>(iteration);

  for (size_t h = 0; h < num_hands_; ++h) {
    double reach_weight = reach_probs[h];
    for (size_t a = 0; a < num_actions_; ++a) {
      size_t i = h * num_actions_ + a;
      cumulative_strategy_sum_[i] += reach_weight * current_weight * current_strategy[i];
    }
  }
  average_strategy_valid_ = false;
}

void DiscountedCfrTrainable::SetEv(const std::vector<double>& evs) {
  size_t total_size = num_actions_ * num_hands_;
  if (evs.size() != total_size) throw std::invalid_argument("EV vector size mismatch in SetEv.");
  expected_values_ = evs;
}

json DiscountedCfrTrainable::DumpStrategy(bool with_ev) const {
  json result = json::object();
  const auto &avg_strategy = GetAverageStrategy();
  
  std::vector<std::vector<double>> strategy_out;
  std::vector<std::vector<double>> evs_out;
  
  bool has_evs = with_ev && !expected_values_.empty();

  for (size_t h = 0; h < num_hands_; ++h) {
    std::vector<double> hand_avg_strategy(num_actions_);

    for (size_t a = 0; a < num_actions_; ++a) {
      size_t index = h * num_actions_ + a;
      if (index < avg_strategy.size()) hand_avg_strategy[a] = avg_strategy[index];
    }
    strategy_out.push_back(hand_avg_strategy);
    
    if (has_evs) {
      std::vector<double> hand_evs(num_actions_, 0.0);
      for (size_t a = 0; a < num_actions_; ++a) {
        size_t index = h * num_actions_ + a;
        if (index < expected_values_.size()) {
          double val = expected_values_[index];
          hand_evs[a] = std::isnan(val) ? 0.0 : val;
        }
      }
      evs_out.push_back(hand_evs);
    }
  }
  result["strategy"] = strategy_out;
  if (has_evs) result["evs"] = evs_out;
  return result;
}

json DiscountedCfrTrainable::DumpEvs() const {
  json result = json::object();
  std::vector<std::vector<double>> evs_out;

  for (size_t h = 0; h < num_hands_; ++h) {
    std::vector<double> hand_evs(num_actions_, std::numeric_limits<double>::quiet_NaN());
    for (size_t a = 0; a < num_actions_; ++a) {
      size_t index = h * num_actions_ + a;
      if (index < expected_values_.size()) hand_evs[a] = expected_values_[index];
    }
    evs_out.push_back(hand_evs);
  }
  result["evs"] = evs_out;
  return result;
}

void DiscountedCfrTrainable::CopyStateFrom(const Trainable& other) {
  const auto *other_dcfr_ptr = dynamic_cast<const DiscountedCfrTrainable *>(&other);
  if (!other_dcfr_ptr) {
    throw std::invalid_argument("Cannot copy state: 'other' is not a DiscountedCfrTrainable.");
  }
  const DiscountedCfrTrainable &other_dcfr = *other_dcfr_ptr;
  if (num_actions_ != other_dcfr.num_actions_ || num_hands_ != other_dcfr.num_hands_) {
    throw std::invalid_argument("Cannot copy state: Dimensions mismatch.");
  }
  this->cumulative_regrets_ = other_dcfr.cumulative_regrets_;
  this->cumulative_strategy_sum_ = other_dcfr.cumulative_strategy_sum_;
  this->current_strategy_ = other_dcfr.current_strategy_;
  this->average_strategy_ = other_dcfr.average_strategy_;
  this->current_strategy_valid_ = other_dcfr.current_strategy_valid_;
  this->average_strategy_valid_ = other_dcfr.average_strategy_valid_;
  this->expected_values_ = other_dcfr.expected_values_;
}

}
}
