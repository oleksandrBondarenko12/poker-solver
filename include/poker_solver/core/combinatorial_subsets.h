#pragma once
#include <vector>

namespace poker_solver {
namespace utils {
template <typename T>
class SimpleCombinations {
public:
    SimpleCombinations(const std::vector<T>& elements, int k) : elements_(elements), k_(k) {
        std::vector<T> current;
        generate(0, current);
    }
    const std::vector<std::vector<T>>& get_combinations() const { return combinations_; }
    const std::vector<std::vector<T>>& GetCombinations() const { return get_combinations(); }
private:
    void generate(size_t start, std::vector<T>& current) {
        if (current.size() == k_) {
            combinations_.push_back(current);
            return;
        }
        for (size_t i = start; i < elements_.size(); ++i) {
            current.push_back(elements_[i]);
            generate(i + 1, current);
            current.pop_back();
        }
    }
    std::vector<T> elements_;
    size_t k_;
    std::vector<std::vector<T>> combinations_;
};

template <typename T>
using CombinatorialSubsets = SimpleCombinations<T>;

} // namespace utils
} // namespace poker_solver
