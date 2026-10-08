#ifndef POKER_SOLVER_UTILS_LIBRARY_H_
#define POKER_SOLVER_UTILS_LIBRARY_H_

#include <vector>
#include <string>
#include <string_view> // Use string_view for efficiency
#include <chrono>
#include <cstdint>
#include <cmath>
#include <limits>  // Required for std::numeric_limits
#include <stdexcept> // Required for std::runtime_error

namespace poker_solver {
namespace utils {

// --- Combinations Class ---

// Generates all combinations of M elements chosen from a given input set.
// Example: Combinations({0, 1, 2, 3}, 2) yields {{0, 1}, {0, 2}, {0, 3},
// {1, 2}, {1, 3}, {2, 3}}.
// Warning: Stores all combinations in memory. Can be memory-intensive for
// large inputs or large combination sizes.
template <typename T>
class Combinations {
 public:
  // Constructs the generator and computes all combinations.
  // Args:
  //   input_set: The vector of elements to choose combinations from.
  //   combination_size: The number of elements in each combination (M).
  Combinations(std::vector<T> input_set, size_t combination_size)
      : input_set_(std::move(input_set)),
        combination_size_(combination_size),
        input_size_(input_set_.size()) {
    if (combination_size_ > input_size_) {
      // Handle invalid input: Cannot choose more elements than available.
      // Resulting combinations_ vector will be empty.
      return;
    }
    if (combination_size_ == 0) {
      // Handle edge case: Choosing 0 elements results in one empty combination.
      combinations_.push_back({});
      return;
    }

    // Pre-calculate the number of combinations to reserve memory.
    size_t num_combinations = CalculateCombinationsCount(input_size_,
                                                         combination_size_);
    if (num_combinations > 0) {
      combinations_.reserve(num_combinations);
      current_combination_.resize(combination_size_);
      GenerateCombinationsRecursive(0, 0);
    }
  }

  // Returns the computed vector of all combinations.
  const std::vector<std::vector<T>>& get_combinations() const {
    return combinations_;
  }
  const std::vector<std::vector<T>>& GetCombinations() const {
    return get_combinations();
  }

  // Provides iterator access to the combinations.
  typename std::vector<std::vector<T>>::const_iterator begin() const {
    return combinations_.begin();
  }
  typename std::vector<std::vector<T>>::const_iterator end() const {
    return combinations_.end();
  }

 private:
  // Recursive helper function to generate combinations.
  // Args:
  //   offset: The starting index in input_set_ for the current level.
  //   k: The current depth in the combination being built (index in
  //      current_combination_).
  void GenerateCombinationsRecursive(size_t offset, size_t k) {
    // Base case: Combination is complete.
    if (k == combination_size_) {
      combinations_.push_back(current_combination_);
      return;
    }

    // Iterate through possible elements to add at the current position.
    // The maximum index to start from is calculated to ensure there are
    // enough remaining elements to complete the combination.
    // size_t remaining_needed = combination_size_ - k;
    // size_t max_index = input_size_ - remaining_needed;
    // Equivalent to: i < input_size_ - (combination_size_ - k) + 1
    // Which simplifies to: i <= input_size_ - combination_size_ + k

    for (size_t i = offset; i <= input_size_ - combination_size_ + k; ++i) {
      current_combination_[k] = input_set_[i];
      GenerateCombinationsRecursive(i + 1, k + 1);
    }
  }


  // Calculates binomial coefficient C(n, k) = n! / (k! * (n-k)!).
  // Uses the multiplicative formula C(n, k) = (n * (n-1) * ... * (n-k+1)) / k!
  // This implementation avoids large intermediate factorials but assumes
  // the final result fits within uint64_t.
  // Args:
  //   n: The total number of items available.
  //   k: The number of items to choose.
  // Returns:
  //   The number of combinations, or 0 if k > n.
  static uint64_t CalculateCombinationsCount(size_t n, size_t k) {
    if (k > n) {
      return 0;
    }
    // Optimization: C(n, k) == C(n, n-k). Choose the smaller k.
    if (k * 2 > n) {
      k = n - k;
    }
    if (k == 0) {
      return 1;
    }

    uint64_t result = 1;
    for (size_t i = 1; i <= k; ++i) {
        // Check for potential overflow before multiplication
        if (result > std::numeric_limits<uint64_t>::max() / (n - i + 1) ) {
             throw std::overflow_error(
                "Overflow detected in CalculateCombinationsCount");
        }
        result *= (n - i + 1) / i; // Perform division at each step
    }
    return result;
  }

  const std::vector<T> input_set_;
  const size_t combination_size_; // M
  const size_t input_size_;       // N
  std::vector<std::vector<T>> combinations_;
  std::vector<T> current_combination_; // Temporary storage during recursion
};

// --- Standalone Utility Functions ---

// Splits a string into a vector of substrings based on a delimiter character.
// Args:
// Splits a string into a vector of substrings based on a delimiter character.
std::vector<std::string> string_split(std::string_view text, char delimiter);
inline std::vector<std::string> StringSplit(std::string_view text, char delimiter) {
  return string_split(text, delimiter);
}

// Gets the current time as milliseconds since the Unix epoch.
uint64_t time_since_epoch_millisec();
inline uint64_t TimeSinceEpochMillisec() {
  return time_since_epoch_millisec();
}

// Generates a pseudo-random integer within a specified range [min, max].
int get_random_int(int min_val, int max_val);
inline int GetRandomInt(int min_val, int max_val) {
  return get_random_int(min_val, max_val);
}

// Normalizes an expected value (ev) relative to a stack size using a
// scaled hyperbolic tangent function. Maps ev to the range [0, 1].
double normalize_tanh(double stack, double ev, double ratio = 7.0);
inline double NormalizeTanh(double stack, double ev, double ratio = 7.0) {
  return normalize_tanh(stack, ev, ratio);
}

} // namespace utils
} // namespace poker_solver

#endif // POKER_SOLVER_UTILS_LIBRARY_H_
