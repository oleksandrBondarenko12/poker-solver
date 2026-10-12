#include "gtest/gtest.h"
#include "poker_solver/evaluator/table_hand_evaluator.h"
#include "poker_solver/core/poker_card.h"
#include <vector>
#include <string>
#include <memory>
#include <stdexcept>
#include <optional>
#include <sstream>

// Use namespaces for convenience
using namespace poker_solver::core;
using namespace poker_solver::eval;

// Test fixture for TableHandEvaluator tests
class TableHandEvaluatorTest : public ::testing::Test {
 protected:
  // Path to the dictionary file (relative to build/executable directory)
  const std::string kDictionaryPath = "five_card_strength.txt";
  std::unique_ptr<TableHandEvaluator> evaluator_;

  // Load the evaluator once for all tests in this fixture
  void SetUp() override {
    try {
      evaluator_ = std::make_unique<TableHandEvaluator>(kDictionaryPath);
    } catch (const std::exception& e) {
      FAIL() << "Failed to load dictionary '" << kDictionaryPath << "': " << e.what();
    }
  }

  // Helper to convert card strings to integer vectors
  std::vector<int> StringsToInts(const std::vector<std::string>& card_strs) {
      std::vector<int> ints;
      ints.reserve(card_strs.size());
      for (const auto& s : card_strs) {
          std::optional<int> opt_int = Card::StringToInt(s);
          if (!opt_int) {
              std::string error_msg = "Invalid card string in test helper: ";
              error_msg += s;
              throw std::runtime_error(error_msg);
          }
          ints.push_back(opt_int.value());
      }
      return ints;
  }

  // Helper for comparing ranks (lower is better)
  void ExpectRankLower(const std::vector<std::string>& hand1_strs,
                       const std::vector<std::string>& hand2_strs,
                       const std::string& message = "") {
        ASSERT_NE(evaluator_, nullptr) << "Evaluator not initialized in test fixture.";
        int rank1 = evaluator_->GetBestRankForCards(StringsToInts(hand1_strs));
        int rank2 = evaluator_->GetBestRankForCards(StringsToInts(hand2_strs));
        ASSERT_NE(rank1, TableHandEvaluator::kInvalidRank) << "Hand 1 evaluated to invalid rank: " << HandVectorToString(hand1_strs);
        ASSERT_NE(rank2, TableHandEvaluator::kInvalidRank) << "Hand 2 evaluated to invalid rank: " << HandVectorToString(hand2_strs);

        EXPECT_LT(rank1, rank2) << message << ": Expected "
            << HandVectorToString(hand1_strs) << " (Rank " << rank1
            << ") to be better than " << HandVectorToString(hand2_strs)
            << " (Rank " << rank2 << ")";
  }

  // Helper to convert hand vector back to string for messages
  std::string HandVectorToString(const std::vector<std::string>& hand_strs) {
      std::ostringstream oss;
      oss << "{";
      for(size_t i = 0; i < hand_strs.size(); ++i) {
          oss << hand_strs[i] << (i == hand_strs.size() - 1 ? "" : ", ");
      }
      oss << "}";
      return oss.str();
  }
};

// --- Static Helper Function Tests ---
TEST(TableHandEvaluatorStaticTest, IsFlushCheck) {
    Card Ah("Ah"), Kh("Kh"), Qh("Qh"), Jh("Jh"), Th("Th"), H5("5h"), H2("2h"), H7("7h");
    Card Ac("Ac"), Kd("Kd"), Js("Js"), Tc("Tc"), Ad("Ad"), Ks("Ks");

    uint64_t flush_h = Card::CardsToUint64({Ah, Kh, H5, H2, H7});
    uint64_t not_flush_str = Card::CardsToUint64({Ac, Kd, Qh, Js, Tc});
    uint64_t not_flush_pair = Card::CardsToUint64({Ac, Ad, Ks, Qh, Js});
    uint64_t four_cards = Card::CardsToUint64({Ah, Kh, H5, H2});
    uint64_t six_cards_flush = Card::CardsToUint64({Ah, Kh, H5, H2, H7, Qh});

    EXPECT_TRUE(TableHandEvaluator::IsFlush(flush_h));
    EXPECT_FALSE(TableHandEvaluator::IsFlush(not_flush_str));
    EXPECT_FALSE(TableHandEvaluator::IsFlush(not_flush_pair));
    EXPECT_FALSE(TableHandEvaluator::IsFlush(four_cards)); // Popcount != 5
    EXPECT_FALSE(TableHandEvaluator::IsFlush(six_cards_flush)); // Popcount != 5
}

TEST(TableHandEvaluatorStaticTest, RanksHashCheck) {
    Card Ac("Ac"), Kd("Kd"), Qh("Qh"), Js("Js"), Tc("Tc");
    Card Ah("Ah"), Ks("Ks"), Qd("Qd"), Jc("Jc"), Th("Th");
    Card Ad("Ad");

    uint64_t hand1_mask = Card::CardsToUint64({Ac, Kd, Qh, Js, Tc});
    uint64_t hand2_mask = Card::CardsToUint64({Ah, Ks, Qd, Jc, Th});
    uint64_t hand3_mask = Card::CardsToUint64({Ac, Ad, Ks, Qh, Js}); // Pair of Aces

    EXPECT_EQ(TableHandEvaluator::RanksHash(hand1_mask), TableHandEvaluator::RanksHash(hand2_mask));
    EXPECT_NE(TableHandEvaluator::RanksHash(hand1_mask), TableHandEvaluator::RanksHash(hand3_mask));
}

// --- Constructor Test ---
TEST(TableHandEvaluatorConstructorTest, FileNotFound) {
    EXPECT_THROW(TableHandEvaluator comp("non_existent_file.txt"), std::runtime_error);
}

// --- Basic GetBestRankForCards Tests ---
TEST_F(TableHandEvaluatorTest, GetBestRankForCards5Cards) {
    ASSERT_NE(evaluator_, nullptr);
    EXPECT_EQ(evaluator_->GetBestRankForCards(StringsToInts({"Ah", "Kh", "Qh", "Jh", "Th"})), 1);
    EXPECT_EQ(evaluator_->GetBestRankForCards(StringsToInts({"As", "Ks", "Qs", "Js", "Ts"})), 1);
    EXPECT_EQ(evaluator_->GetBestRankForCards(StringsToInts({"Kc", "Qc", "Jc", "Tc", "9c"})), 2);
    EXPECT_EQ(evaluator_->GetBestRankForCards(StringsToInts({"Qd", "Jd", "Td", "9d", "8d"})), 3);
    EXPECT_EQ(evaluator_->GetBestRankForCards(StringsToInts({"2s", "3s", "4h", "5s", "7h"})), 7462);
    EXPECT_EQ(evaluator_->GetBestRankForCards({}), TableHandEvaluator::kInvalidRank);
}

TEST_F(TableHandEvaluatorTest, GetBestRankForCards7Cards) {
    ASSERT_NE(evaluator_, nullptr);
    EXPECT_EQ(evaluator_->GetBestRankForCards(StringsToInts({"Ah", "Kh", "Qh", "Jh", "Th", "2c", "3d"})), 1);
    EXPECT_EQ(evaluator_->GetBestRankForCards(StringsToInts({"Kc", "Qc", "Jc", "Tc", "9c", "As", "Ad"})), 2);
    // Check flush rank calculation
    int rank_7card_flush = evaluator_->GetBestRankForCards(StringsToInts({"Ah", "Kh", "5h", "2h", "7h", "Qh", "3d"}));
    int rank_best5_flush = evaluator_->GetBestRankForCards(StringsToInts({"Ah", "Kh", "Qh", "7h", "5h"}));
    ASSERT_NE(rank_best5_flush, TableHandEvaluator::kInvalidRank) << "Best 5-card flush rank is invalid";
    EXPECT_EQ(rank_7card_flush, rank_best5_flush);

    // Check straight rank calculation
    int rank_7card_straight = evaluator_->GetBestRankForCards(StringsToInts({"Ac", "Kd", "Qh", "Js", "Tc", "2c", "3d"}));
    int rank_best5_straight = evaluator_->GetBestRankForCards(StringsToInts({"Ac", "Kd", "Qh", "Js", "Tc"}));
    ASSERT_NE(rank_best5_straight, TableHandEvaluator::kInvalidRank) << "Best 5-card straight rank is invalid";
    EXPECT_EQ(rank_7card_straight, rank_best5_straight);

    // Check two-pair rank calculation
    int rank_7card_twopair = evaluator_->GetBestRankForCards(StringsToInts({"Ac", "Ad", "Ks", "Kh", "2c", "3d", "4h"}));
    int rank_best5_twopair = evaluator_->GetBestRankForCards(StringsToInts({"Ac", "Ad", "Ks", "Kh", "4h"}));
    ASSERT_NE(rank_best5_twopair, TableHandEvaluator::kInvalidRank) << "Best 5-card two-pair rank is invalid";
    EXPECT_EQ(rank_7card_twopair, rank_best5_twopair);
}

// --- Public Interface Tests ---
TEST_F(TableHandEvaluatorTest, GetHandRankMasks) {
    ASSERT_NE(evaluator_, nullptr);
    Card card_2c("2c"); Card card_3d("3d"); Card card_Ah("Ah"); Card card_Kh("Kh");
    Card card_Qh("Qh"); Card card_Jh("Jh"); Card card_Th("Th"); Card card_Qd("Qd");

    uint64_t board = Card::CardsToUint64({card_2c, card_3d});
    uint64_t private_rf = Card::CardsToUint64({card_Ah, card_Kh});
    uint64_t public_rf = Card::CardsToUint64({card_Qh, card_Jh, card_Th});
    EXPECT_EQ(evaluator_->GetHandRank(private_rf, public_rf), 1);
    uint64_t private_overlap = Card::CardsToUint64({card_Ah, card_Qh});
    EXPECT_EQ(evaluator_->GetHandRank(private_overlap, public_rf), TableHandEvaluator::kInvalidRank);
}

TEST_F(TableHandEvaluatorTest, CompareHandsVectors) {
    ASSERT_NE(evaluator_, nullptr);
    std::vector<int> p1_royal = StringsToInts({"Ah", "Kh"});
    std::vector<int> p2_quads = StringsToInts({"Ac", "Ad"});
    std::vector<int> p2_lower_sf = StringsToInts({"9s", "8s"});
    std::vector<int> board_rf = StringsToInts({"Qh", "Jh", "Th", "2c", "3d"});
    std::vector<int> board_quads = StringsToInts({"As", "Ah", "Kc", "Kd", "2d"});
    std::vector<int> board_sf = StringsToInts({"7s", "6s", "5s", "Ad", "Ac"});
    EXPECT_EQ(evaluator_->CompareHands(p1_royal, p2_quads, board_rf), ComparisonResult::kPlayer1Wins);
    EXPECT_EQ(evaluator_->CompareHands(p1_royal, p2_lower_sf, board_rf), ComparisonResult::kPlayer1Wins);
    EXPECT_EQ(evaluator_->CompareHands(p2_lower_sf, p2_quads, board_sf), ComparisonResult::kPlayer1Wins);
    std::vector<int> p1_tie = StringsToInts({"2c", "3c"});
    std::vector<int> p2_tie = StringsToInts({"2d", "3d"});
    std::vector<int> board_tie = StringsToInts({"Ah", "Ks", "Qd", "Jc", "Th"});
    EXPECT_EQ(evaluator_->CompareHands(p1_tie, p2_tie, board_tie), ComparisonResult::kTie);
    std::vector<int> p1_overlap = StringsToInts({"Ah", "Kh"});
    std::vector<int> p2_overlap = StringsToInts({"Ah", "Qd"});
    EXPECT_EQ(evaluator_->CompareHands(p1_overlap, p2_overlap, board_rf), ComparisonResult::kTie);
}

TEST_F(TableHandEvaluatorTest, CompareHandsMasks) {
    ASSERT_NE(evaluator_, nullptr);
    Card card_Ah("Ah"); Card card_Kh("Kh"); Card card_Qh("Qh"); Card card_Jh("Jh");
    Card card_Th("Th"); Card card_2c("2c"); Card card_3d("3d"); Card card_Ac("Ac");
    Card card_Ad("Ad"); Card card_Qd("Qd"); Card card_As("As"); Card card_Kc("Kc");
    Card card_Kd("Kd"); Card card_2d("2d");
    uint64_t p1_royal_mask = Card::CardsToUint64({card_Ah, card_Kh});
    uint64_t p2_quads_mask = Card::CardsToUint64({card_Ac, card_Ad});
    uint64_t board_rf_mask = Card::CardsToUint64({card_Qh, card_Jh, card_Th, card_2c, card_3d});
    uint64_t board_quads_mask = Card::CardsToUint64({card_As, card_Ah, card_Kc, card_Kd, card_2d});
    EXPECT_EQ(evaluator_->CompareHands(p1_royal_mask, p2_quads_mask, board_rf_mask), ComparisonResult::kPlayer1Wins);
    uint64_t p2_overlap_mask = Card::CardsToUint64({card_Ah, card_Qd});
    EXPECT_EQ(evaluator_->CompareHands(p1_royal_mask, p2_overlap_mask, board_rf_mask), ComparisonResult::kTie);
}

// --- Comprehensive Rank Category Comparison Test ---
TEST_F(TableHandEvaluatorTest, CompareRankCategories) {
    ASSERT_NE(evaluator_, nullptr);
    auto rf = StringsToInts({"Ah", "Kh", "Qh", "Jh", "Th"});
    auto sf = StringsToInts({"9d", "8d", "7d", "6d", "5d"});
    auto quads = StringsToInts({"7c", "7d", "7h", "7s", "Kc"});
    auto fh = StringsToInts({"Tc", "Td", "Th", "2s", "2c"});
    auto flush = StringsToInts({"Ac", "Qc", "Tc", "5c", "2c"});
    auto straight = StringsToInts({"Ac", "Kd", "Qh", "Js", "Tc"});
    auto trips = StringsToInts({"5c", "5d", "5h", "Ks", "Qh"});
    auto two_pair = StringsToInts({"Ac", "Ad", "Ks", "Kh", "Qh"});
    auto one_pair = StringsToInts({"Ac", "Ad", "Ks", "Qh", "Js"});
    auto high_card = StringsToInts({"Ac", "Kd", "Qh", "Js", "9h"});

    int rank_rf = evaluator_->GetBestRankForCards(rf);
    int rank_sf = evaluator_->GetBestRankForCards(sf);
    int rank_quads = evaluator_->GetBestRankForCards(quads);
    int rank_fh = evaluator_->GetBestRankForCards(fh);
    int rank_flush = evaluator_->GetBestRankForCards(flush);
    int rank_straight = evaluator_->GetBestRankForCards(straight);
    int rank_trips = evaluator_->GetBestRankForCards(trips);
    int rank_two_pair = evaluator_->GetBestRankForCards(two_pair);
    int rank_one_pair = evaluator_->GetBestRankForCards(one_pair);
    int rank_high_card = evaluator_->GetBestRankForCards(high_card);

    ASSERT_NE(rank_rf, TableHandEvaluator::kInvalidRank) << "RF rank invalid";
    ASSERT_NE(rank_sf, TableHandEvaluator::kInvalidRank) << "SF rank invalid";
    ASSERT_NE(rank_quads, TableHandEvaluator::kInvalidRank) << "Quads rank invalid";
    ASSERT_NE(rank_fh, TableHandEvaluator::kInvalidRank) << "FH rank invalid";
    ASSERT_NE(rank_flush, TableHandEvaluator::kInvalidRank) << "Flush rank invalid";
    ASSERT_NE(rank_straight, TableHandEvaluator::kInvalidRank) << "Straight rank invalid";
    ASSERT_NE(rank_trips, TableHandEvaluator::kInvalidRank) << "Trips rank invalid";
    ASSERT_NE(rank_two_pair, TableHandEvaluator::kInvalidRank) << "Two Pair rank invalid";
    ASSERT_NE(rank_one_pair, TableHandEvaluator::kInvalidRank) << "One Pair rank invalid";
    ASSERT_NE(rank_high_card, TableHandEvaluator::kInvalidRank) << "High Card rank invalid";

    EXPECT_LT(rank_rf, rank_sf) << "Royal Flush vs Straight Flush";
    EXPECT_LT(rank_sf, rank_quads) << "Straight Flush vs Quads";
    EXPECT_LT(rank_quads, rank_fh) << "Quads vs Full House";
    EXPECT_LT(rank_fh, rank_flush) << "Full House vs Flush";
    EXPECT_LT(rank_flush, rank_straight) << "Flush vs Straight";
    EXPECT_LT(rank_straight, rank_trips) << "Straight vs Trips";
    EXPECT_LT(rank_trips, rank_two_pair) << "Trips vs Two Pair";
    EXPECT_LT(rank_two_pair, rank_one_pair) << "Two Pair vs One Pair";
    EXPECT_LT(rank_one_pair, rank_high_card) << "One Pair vs High Card";
}

// --- Within-Rank Comparison Tests ---

TEST_F(TableHandEvaluatorTest, CompareWithinStraightFlush) {
    ASSERT_NE(evaluator_, nullptr);
    ExpectRankLower({"Kh", "Qh", "Jh", "Th", "9h"}, // K-high SF
                    {"Qh", "Jh", "Th", "9h", "8h"}, // Q-high SF
                    "K-high SF vs Q-high SF");
}

TEST_F(TableHandEvaluatorTest, CompareWithinQuads) {
     ASSERT_NE(evaluator_, nullptr);
     ExpectRankLower({"Ah", "Ad", "Ac", "As", "2h"}, // Quads Aces
                     {"Kh", "Kd", "Kc", "Ks", "Ah"}, // Quads Kings
                     "Quads Aces vs Quads Kings");
     ExpectRankLower({"Kh", "Kd", "Kc", "Ks", "Ah"}, // Quads Kings, Ace kicker
                     {"Kh", "Kd", "Kc", "Ks", "Qh"}, // Quads Kings, Queen kicker
                     "Quads Kings (A kicker) vs Quads Kings (Q kicker)");
}

TEST_F(TableHandEvaluatorTest, CompareWithinFullHouse) {
     ASSERT_NE(evaluator_, nullptr);
     ExpectRankLower({"Ah", "Ad", "Ac", "2s", "2h"}, // Aces full of Twos
                     {"Kh", "Kd", "Kc", "As", "Ah"}, // Kings full of Aces
                     "AAA22 vs KKKAA");
      ExpectRankLower({"Kh", "Kd", "Kc", "As", "Ah"}, // Kings full of Aces
                      {"Kh", "Kd", "Kc", "Qs", "Qh"}, // Kings full of Queens
                      "KKKAA vs KKKQQ");
}

TEST_F(TableHandEvaluatorTest, CompareWithinFlush) {
     ASSERT_NE(evaluator_, nullptr);
     ExpectRankLower({"Ah", "Kh", "Qh", "Jh", "9h"}, // A K Q J 9 Flush
                     {"Ah", "Kh", "Qh", "Th", "9h"}, // A K Q T 9 Flush
                     "AKQJ9 Flush vs AKQT9 Flush");
     ExpectRankLower({"Kh", "Qh", "Jh", "Th", "8h"}, // K Q J T 8 Flush
                     {"Kh", "Qh", "Jh", "9h", "8h"}, // K Q J 9 8 Flush
                     "KQJT8 Flush vs KQJ98 Flush");
}

TEST_F(TableHandEvaluatorTest, CompareWithinStraight) {
      ASSERT_NE(evaluator_, nullptr);
      ExpectRankLower({"Ah", "Kd", "Qc", "Js", "Th"}, // Ace high Straight
                      {"Kh", "Qd", "Jc", "Ts", "9h"}, // King high Straight
                      "AKQJT Straight vs KQJT9 Straight");

      // Test Wheel vs 6-high explicitly using EXPECT_GT on the rank numbers
      int rank_wheel = evaluator_->GetBestRankForCards(StringsToInts({"5h", "4d", "3c", "2s", "Ah"}));
      int rank_6high = evaluator_->GetBestRankForCards(StringsToInts({"6h", "5d", "4c", "3s", "2h"}));
      ASSERT_NE(rank_wheel, TableHandEvaluator::kInvalidRank) << "Wheel rank invalid";
      ASSERT_NE(rank_6high, TableHandEvaluator::kInvalidRank) << "6-high straight rank invalid";
      EXPECT_GT(rank_wheel, rank_6high) << "Wheel (Rank " << rank_wheel
                                        << ") should be worse (higher rank number) than 6-high (Rank "
                                        << rank_6high << ")";
}

TEST_F(TableHandEvaluatorTest, CompareWithinTrips) {
      ASSERT_NE(evaluator_, nullptr);
      ExpectRankLower({"Ah", "Ad", "Ac", "Ks", "Qh"}, // Trip Aces
                      {"Kh", "Kd", "Kc", "As", "Qh"}, // Trip Kings
                      "Trip Aces vs Trip Kings");
      ExpectRankLower({"Ah", "Ad", "Ac", "Ks", "Qh"}, // Trip Aces, K Q kicker
                      {"Ah", "Ad", "Ac", "Ks", "Jh"}, // Trip Aces, K J kicker
                      "Trip Aces (KQ kicker) vs Trip Aces (KJ kicker)");
       ExpectRankLower({"Ah", "Ad", "Ac", "Ks", "Jh"}, // Trip Aces, K J kicker
                       {"Ah", "Ad", "Ac", "Qs", "Jh"}, // Trip Aces, Q J kicker
                       "Trip Aces (KJ kicker) vs Trip Aces (QJ kicker)");
}

TEST_F(TableHandEvaluatorTest, CompareWithinTwoPair) {
     ASSERT_NE(evaluator_, nullptr);
     ExpectRankLower({"Ah", "Ad", "Ks", "Kh", "Qh"}, // Aces and Kings
                     {"Ah", "Ad", "Qs", "Qh", "Ks"}, // Aces and Queens
                     "AAKKQ vs AAQQK");
     ExpectRankLower({"Ah", "Ad", "Qs", "Qh", "Ks"}, // Aces and Queens, K kicker
                     {"Ah", "Ad", "Qs", "Qh", "Js"}, // Aces and Queens, J kicker
                     "AAQQK vs AAQQJ");
}

TEST_F(TableHandEvaluatorTest, CompareWithinOnePair) {
      ASSERT_NE(evaluator_, nullptr);
      ExpectRankLower({"Ah", "Ad", "Ks", "Qh", "Js"}, // Pair Aces
                      {"Kh", "Kd", "As", "Qh", "Js"}, // Pair Kings
                      "Pair Aces vs Pair Kings");
      ExpectRankLower({"Ah", "Ad", "Ks", "Qh", "Js"}, // Pair Aces, K Q J kicker
                      {"Ah", "Ad", "Ks", "Qh", "Ts"}, // Pair Aces, K Q T kicker
                      "Pair Aces (KQJ kicker) vs Pair Aces (KQT kicker)");
}

TEST_F(TableHandEvaluatorTest, CompareWithinHighCard) {
     ASSERT_NE(evaluator_, nullptr);
     ExpectRankLower({"Ah", "Ks", "Qd", "Jc", "9h"}, // Ace high, K Q J 9
                     {"Ah", "Ks", "Qd", "Tc", "9h"}, // Ace high, K Q T 9
                     "AKQJ9 HC vs AKQT9 HC");
     ExpectRankLower({"Kh", "Qs", "Jd", "9c", "7h"}, // King high
                     {"Kh", "Qs", "Jd", "8c", "7h"}, // King high (lower kicker)
                     "KQJ97 HC vs KQJ87 HC");
}
