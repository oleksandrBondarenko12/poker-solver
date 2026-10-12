#include "gtest/gtest.h"
#include "poker_solver/core/canonical_flop_isomorphism.h"
#include "poker_solver/core/poker_card.h"
#include <numeric>
#include <set>
#include <array>

namespace poker_solver {
namespace utils {

class CanonicalFlopIsomorphismTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 1. Total number of canonical flops must be exactly 1,755
TEST_F(CanonicalFlopIsomorphismTest, CanonicalFlopCount) {
    const auto& canonical_flops = FlopIsomorphism::GetCanonicalFlops();
    EXPECT_EQ(canonical_flops.size(), 1755u);
}

// 2. Sum of weights across all canonical flops must be exactly 22,100 (C(52, 3))
TEST_F(CanonicalFlopIsomorphismTest, TotalWeightSum) {
    const auto& canonical_flops = FlopIsomorphism::GetCanonicalFlops();
    double total_weight = 0.0;
    for (const auto& flop : canonical_flops) {
        total_weight += flop.second;
    }
    EXPECT_DOUBLE_EQ(total_weight, 22100.0);
}

// 3. Permutation counts: sum of counts per canonical flop equals its weight,
// and grand total equals 22,100.
TEST_F(CanonicalFlopIsomorphismTest, PermutationCountsConsistency) {
    const auto& canonical_flops = FlopIsomorphism::GetCanonicalFlops();
    const auto& perm_counts = FlopIsomorphism::GetCanonicalFlopPermCounts();

    ASSERT_EQ(canonical_flops.size(), perm_counts.size());

    int grand_total = 0;
    for (size_t i = 0; i < canonical_flops.size(); ++i) {
        int sum_counts = 0;
        for (int p = 0; p < 24; ++p) {
            EXPECT_GE(perm_counts[i][p], 0);
            sum_counts += perm_counts[i][p];
        }
        EXPECT_EQ(sum_counts, static_cast<int>(canonical_flops[i].second));
        grand_total += sum_counts;
    }
    EXPECT_EQ(grand_total, 22100);
}

// 4. All 24 permutations are valid permutations of {0, 1, 2, 3}
TEST_F(CanonicalFlopIsomorphismTest, AllPermutationsAreValidBijection) {
    std::set<std::array<int, 4>> unique_perms;
    for (int p = 0; p < 24; ++p) {
        const auto& perm = FlopIsomorphism::GetPermutationByIndex(p);
        std::set<int> elements(perm.begin(), perm.end());
        EXPECT_EQ(elements.size(), 4u);
        for (int s = 0; s < 4; ++s) {
            EXPECT_TRUE(elements.count(s) > 0);
        }
        unique_perms.insert(perm);
    }
    EXPECT_EQ(unique_perms.size(), 24u);
}

// 5. Canonical board transformation is idempotent
TEST_F(CanonicalFlopIsomorphismTest, CanonicalBoardIsIdempotent) {
    const auto& canonical_flops = FlopIsomorphism::GetCanonicalFlops();
    for (const auto& flop : canonical_flops) {
        uint64_t canon_again = FlopIsomorphism::GetCanonicalBoard(flop.first);
        EXPECT_EQ(canon_again, flop.first);
    }
}

} // namespace utils
} // namespace poker_solver
