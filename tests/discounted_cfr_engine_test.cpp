#include "gtest/gtest.h"
#include "poker_solver/solver/discounted_cfr_engine.h"
#include <cmath>
#include <vector>
#include <numeric>

namespace poker_solver {
namespace solver {

class DiscountedCfrTrainableTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// 1. Initialization: Strategy must be uniform 1/|A| and sum to 1.0 for all hands.
TEST_F(DiscountedCfrTrainableTest, InitializationUniformStrategy) {
    const size_t num_actions = 3;
    const size_t num_hands = 4;
    DiscountedCfrTrainable trainable(num_actions, num_hands);

    const auto& current_strat = trainable.GetCurrentStrategy();
    const auto& avg_strat = trainable.GetAverageStrategy();

    ASSERT_EQ(current_strat.size(), num_actions * num_hands);
    ASSERT_EQ(avg_strat.size(), num_actions * num_hands);

    for (size_t h = 0; h < num_hands; ++h) {
        double current_sum = 0.0;
        double avg_sum = 0.0;
        for (size_t a = 0; a < num_actions; ++a) {
            size_t idx = h * num_actions + a;
            EXPECT_NEAR(current_strat[idx], 1.0 / 3.0, 1e-9);
            EXPECT_NEAR(avg_strat[idx], 1.0 / 3.0, 1e-9);
            current_sum += current_strat[idx];
            avg_sum += avg_strat[idx];
        }
        EXPECT_NEAR(current_sum, 1.0, 1e-9);
        EXPECT_NEAR(avg_sum, 1.0, 1e-9);
    }
}

// 2. Regret Matching+ and Clamping: Negative regrets must be floored at 0.
TEST_F(DiscountedCfrTrainableTest, RegretMatchingPlusAndClamping) {
    const size_t num_actions = 3;
    const size_t num_hands = 1;
    DiscountedCfrTrainable trainable(num_actions, num_hands);

    // Iteration 1: Action 0 gains +4.0, Action 1 gains +1.0, Action 2 loses -5.0
    std::vector<double> regrets_iter1 = {4.0, 1.0, -5.0};
    trainable.UpdateRegrets(regrets_iter1, 1);

    const auto& strat1 = trainable.GetCurrentStrategy();
    // Sum of positive regrets: 4.0 + 1.0 + 0.0 = 5.0
    // Strategy: [4/5, 1/5, 0] = [0.8, 0.2, 0.0]
    EXPECT_NEAR(strat1[0], 0.8, 1e-9);
    EXPECT_NEAR(strat1[1], 0.2, 1e-9);
    EXPECT_NEAR(strat1[2], 0.0, 1e-9);

    // Iteration 2: Action 0 loses -6.0, Action 1 loses -0.5, Action 2 gains +2.0
    // Cumulative regrets before clamping:
    // Action 0: 4.0 - 6.0 = -2.0 -> clamped to 0.0
    // Action 1: 1.0 - 0.5 = +0.5 -> 0.5
    // Action 2: 0.0 + 2.0 = +2.0 -> 2.0
    std::vector<double> regrets_iter2 = {-6.0, -0.5, 2.0};
    trainable.UpdateRegrets(regrets_iter2, 2);

    const auto& strat2 = trainable.GetCurrentStrategy();
    // Sum of positive regrets: 0.0 + 0.5 + 2.0 = 2.5
    // Strategy: [0/2.5, 0.5/2.5, 2.0/2.5] = [0.0, 0.2, 0.8]
    EXPECT_NEAR(strat2[0], 0.0, 1e-9);
    EXPECT_NEAR(strat2[1], 0.2, 1e-9);
    EXPECT_NEAR(strat2[2], 0.8, 1e-9);
}

// 3. All non-positive regrets fallback to uniform distribution
TEST_F(DiscountedCfrTrainableTest, AllNonPositiveRegretsFallbackToUniform) {
    const size_t num_actions = 2;
    const size_t num_hands = 1;
    DiscountedCfrTrainable trainable(num_actions, num_hands);

    // Update with negative regrets: cumulative clamped to 0
    std::vector<double> negative_regrets = {-2.0, -3.0};
    trainable.UpdateRegrets(negative_regrets, 1);

    const auto& strat = trainable.GetCurrentStrategy();
    // Since cumulative regrets are all 0, must fallback to uniform 0.5 each
    EXPECT_NEAR(strat[0], 0.5, 1e-9);
    EXPECT_NEAR(strat[1], 0.5, 1e-9);
}

// 4. Linear Average Strategy Accumulation with Iteration Weighting & Hero Reach Weighting
TEST_F(DiscountedCfrTrainableTest, LinearWeightingAndHeroReach) {
    const size_t num_actions = 2;
    const size_t num_hands = 2;
    DiscountedCfrTrainable trainable(num_actions, num_hands);

    // Iteration 1 (t = 1):
    // Hand 0 reach = 1.0, Hand 1 reach = 0.5
    // Strategy: Hand 0 plays [0.8, 0.2], Hand 1 plays [0.4, 0.6]
    double reach_iter1[2] = {1.0, 0.5};
    std::vector<double> strat_iter1 = {0.8, 0.2, 0.4, 0.6};
    trainable.AccumulateAverageStrategy(strat_iter1, reach_iter1, 1);

    // Iteration 2 (t = 2):
    // Hand 0 reach = 0.5, Hand 1 reach = 1.0
    // Strategy: Hand 0 plays [0.2, 0.8], Hand 1 plays [0.9, 0.1]
    double reach_iter2[2] = {0.5, 1.0};
    std::vector<double> strat_iter2 = {0.2, 0.8, 0.9, 0.1};
    trainable.AccumulateAverageStrategy(strat_iter2, reach_iter2, 2);

    // Theoretical expected cumulative sums:
    // Hand 0:
    //   t=1: weight = 1 * reach 1.0 = 1.0 -> [1.0 * 0.8, 1.0 * 0.2] = [0.8, 0.2]
    //   t=2: weight = 2 * reach 0.5 = 1.0 -> [1.0 * 0.2, 1.0 * 0.8] = [0.2, 0.8]
    //   total sum = [1.0, 1.0] -> avg = [0.5, 0.5]
    // Hand 1:
    //   t=1: weight = 1 * reach 0.5 = 0.5 -> [0.5 * 0.4, 0.5 * 0.6] = [0.2, 0.3]
    //   t=2: weight = 2 * reach 1.0 = 2.0 -> [2.0 * 0.9, 2.0 * 0.1] = [1.8, 0.2]
    //   total sum = [2.0, 0.5] (sum = 2.5) -> avg = [2.0/2.5, 0.5/2.5] = [0.8, 0.2]
    const auto& avg_strat = trainable.GetAverageStrategy();

    // Check Hand 0
    EXPECT_NEAR(avg_strat[0], 0.5, 1e-9);
    EXPECT_NEAR(avg_strat[1], 0.5, 1e-9);

    // Check Hand 1
    EXPECT_NEAR(avg_strat[2], 0.8, 1e-9);
    EXPECT_NEAR(avg_strat[3], 0.2, 1e-9);
}

// 5. Unreached hand defaults to uniform distribution
TEST_F(DiscountedCfrTrainableTest, UnreachedHandDefaultsToUniform) {
    const size_t num_actions = 3;
    const size_t num_hands = 2;
    DiscountedCfrTrainable trainable(num_actions, num_hands);

    // Hand 0 has reach 1.0, Hand 1 has reach 0.0
    double reach[2] = {1.0, 0.0};
    std::vector<double> strat = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    trainable.AccumulateAverageStrategy(strat, reach, 1);

    const auto& avg = trainable.GetAverageStrategy();
    // Hand 0 was reached: played action 0 100%
    EXPECT_NEAR(avg[0], 1.0, 1e-9);
    EXPECT_NEAR(avg[1], 0.0, 1e-9);
    EXPECT_NEAR(avg[2], 0.0, 1e-9);

    // Hand 1 had reach 0.0 (never reached): must default to uniform 1/3
    EXPECT_NEAR(avg[3], 1.0 / 3.0, 1e-9);
    EXPECT_NEAR(avg[4], 1.0 / 3.0, 1e-9);
    EXPECT_NEAR(avg[5], 1.0 / 3.0, 1e-9);
}

// 6. CopyStateFrom creates a completely identical, decoupled copy
TEST_F(DiscountedCfrTrainableTest, CopyStateFrom) {
    const size_t num_actions = 2;
    const size_t num_hands = 1;
    DiscountedCfrTrainable orig(num_actions, num_hands);

    std::vector<double> regrets = {3.0, 1.0};
    orig.UpdateRegrets(regrets, 1);
    double reach[1] = {0.75};
    orig.AccumulateAverageStrategy(orig.GetCurrentStrategy(), reach, 1);

    DiscountedCfrTrainable copy(num_actions, num_hands);
    copy.CopyStateFrom(orig);

    EXPECT_EQ(copy.GetCurrentStrategy(), orig.GetCurrentStrategy());
    EXPECT_EQ(copy.GetAverageStrategy(), orig.GetAverageStrategy());

    // Mutate original, copy must not change
    std::vector<double> more_regrets = {0.0, 5.0};
    orig.UpdateRegrets(more_regrets, 2);

    EXPECT_NE(copy.GetCurrentStrategy(), orig.GetCurrentStrategy());
}

// 7. Error handling on dimension mismatch and invalid iteration number
TEST_F(DiscountedCfrTrainableTest, ErrorHandling) {
    DiscountedCfrTrainable trainable(2, 2);

    // Invalid size for UpdateRegrets
    EXPECT_THROW(trainable.UpdateRegrets({1.0, 2.0}, 1), std::invalid_argument);

    // Invalid iteration <= 0
    EXPECT_THROW(trainable.UpdateRegrets({1.0, 2.0, 3.0, 4.0}, 0), std::invalid_argument);
    EXPECT_THROW(trainable.UpdateRegrets({1.0, 2.0, 3.0, 4.0}, -1), std::invalid_argument);

    // SetEv size mismatch
    EXPECT_THROW(trainable.SetEv({1.0}), std::invalid_argument);

    // AccumulateAverageStrategy size mismatch, null pointer, invalid iteration
    double reach[2] = {1.0, 1.0};
    EXPECT_THROW(trainable.AccumulateAverageStrategy({1.0, 2.0}, reach, 1), std::invalid_argument);
    EXPECT_THROW(trainable.AccumulateAverageStrategy({1.0, 2.0, 3.0, 4.0}, nullptr, 1), std::invalid_argument);
    EXPECT_THROW(trainable.AccumulateAverageStrategy({1.0, 2.0, 3.0, 4.0}, reach, 0), std::invalid_argument);
}

// 8. Tiny positive regret (< 1e-12) preserves non-uniform probabilities
TEST_F(DiscountedCfrTrainableTest, TinyPositiveRegretPreserved) {
    DiscountedCfrTrainable trainable(2, 1);
    // Extremely small positive regret on action 0, 0 on action 1
    std::vector<double> regrets = {1e-15, 0.0};
    trainable.UpdateRegrets(regrets, 1);

    const auto& strat = trainable.GetCurrentStrategy();
    EXPECT_NEAR(strat[0], 1.0, 1e-9);
    EXPECT_NEAR(strat[1], 0.0, 1e-9);

    double reach[1] = {1e-15};
    trainable.AccumulateAverageStrategy(strat, reach, 1);
    const auto& avg = trainable.GetAverageStrategy();
    EXPECT_NEAR(avg[0], 1.0, 1e-9);
    EXPECT_NEAR(avg[1], 0.0, 1e-9);
}

// 9. DumpStrategy and DumpEvs consistency
TEST_F(DiscountedCfrTrainableTest, DumpStrategyAndDumpEvs) {
    DiscountedCfrTrainable trainable(2, 1);

    // Dump before SetEv: DumpStrategy without ev should have no "evs" key
    json dump_no_ev = trainable.DumpStrategy(false);
    EXPECT_FALSE(dump_no_ev.contains("evs"));

    // Dump with ev before SetEv: should not emit fake evs
    json dump_with_ev_unset = trainable.DumpStrategy(true);
    EXPECT_FALSE(dump_with_ev_unset.contains("evs"));

    // Dump after SetEv
    trainable.SetEv({10.5, -5.2});
    json dump_with_ev_set = trainable.DumpStrategy(true);
    ASSERT_TRUE(dump_with_ev_set.contains("evs"));
    EXPECT_NEAR(dump_with_ev_set["evs"][0][0].get<double>(), 10.5, 1e-6);
    EXPECT_NEAR(dump_with_ev_set["evs"][0][1].get<double>(), -5.2, 1e-6);

    json dump_evs = trainable.DumpEvs();
    ASSERT_TRUE(dump_evs.contains("evs"));
    EXPECT_NEAR(dump_evs["evs"][0][0].get<double>(), 10.5, 1e-6);
    EXPECT_NEAR(dump_evs["evs"][0][1].get<double>(), -5.2, 1e-6);
}

} // namespace solver
} // namespace poker_solver
