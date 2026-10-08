#include "gtest/gtest.h"
#include "poker_solver/tree/tree_building_config.h" // Adjust path if needed
#include "poker_solver/tree/street_betting_rule.h"             // Adjust path if needed
#include "poker_solver/tree/game_tree_node_types.h"               // For GameRound enum
#include "poker_solver/tree/extensive_game_tree.h"
#include "poker_solver/tree/scenario_game_rule.h"
#include "poker_solver/core/card_deck.h"
#include <vector>
#include <stdexcept>
#include <memory> // For std::unique_ptr

// Use namespaces
using namespace poker_solver;
using namespace poker_solver::core;
using namespace poker_solver::config;

// Test fixture for GameTreeBuildingSettings tests
class GameTreeBuildingSettingsTest : public ::testing::Test {
 protected:
  // Create distinct StreetSetting objects for testing
  StreetSetting flop_ip_{{33.0}, {50.0}, {}, false};
  StreetSetting turn_ip_{{50.0}, {75.0}, {}, true};
  StreetSetting river_ip_{{75.0}, {100.0}, {}, true};
  StreetSetting flop_oop_{{25.0}, {}, {50.0}, false}; // Has donk
  StreetSetting turn_oop_{{50.0}, {100.0}, {}, true};
  StreetSetting river_oop_{{100.0}, {150.0}, {}, true};

  // Initialize GameTreeBuildingSettings in SetUp
  std::unique_ptr<GameTreeBuildingSettings> settings_;

  void SetUp() override {
    // Pass copies to the constructor
    settings_ = std::make_unique<GameTreeBuildingSettings>(
        flop_ip_, turn_ip_, river_ip_,
        flop_oop_, turn_oop_, river_oop_
    );
  }
};

// Test the GetSetting method for valid inputs
TEST_F(GameTreeBuildingSettingsTest, GetSettingValid) {
  ASSERT_NE(settings_, nullptr);

  // --- Verify by comparing content, not addresses ---

  // Player 0 (IP)
  const StreetSetting& actual_flop_ip = settings_->GetSetting(0, GameRound::kFlop);
  EXPECT_EQ(actual_flop_ip.bet_sizes_percent, flop_ip_.bet_sizes_percent);
  EXPECT_EQ(actual_flop_ip.raise_sizes_percent, flop_ip_.raise_sizes_percent);
  EXPECT_EQ(actual_flop_ip.allow_all_in, flop_ip_.allow_all_in);

  const StreetSetting& actual_turn_ip = settings_->GetSetting(0, GameRound::kTurn);
  EXPECT_EQ(actual_turn_ip.bet_sizes_percent, turn_ip_.bet_sizes_percent);
  EXPECT_EQ(actual_turn_ip.raise_sizes_percent, turn_ip_.raise_sizes_percent);
  EXPECT_EQ(actual_turn_ip.allow_all_in, turn_ip_.allow_all_in);

  const StreetSetting& actual_river_ip = settings_->GetSetting(0, GameRound::kRiver);
  EXPECT_EQ(actual_river_ip.bet_sizes_percent, river_ip_.bet_sizes_percent);
  EXPECT_EQ(actual_river_ip.raise_sizes_percent, river_ip_.raise_sizes_percent);
  EXPECT_EQ(actual_river_ip.allow_all_in, river_ip_.allow_all_in);


  // Player 1 (OOP)
  const StreetSetting& actual_flop_oop = settings_->GetSetting(1, GameRound::kFlop);
  EXPECT_EQ(actual_flop_oop.bet_sizes_percent, flop_oop_.bet_sizes_percent);
  EXPECT_EQ(actual_flop_oop.donk_sizes_percent, flop_oop_.donk_sizes_percent); // Check donk
  EXPECT_EQ(actual_flop_oop.allow_all_in, flop_oop_.allow_all_in);

  const StreetSetting& actual_turn_oop = settings_->GetSetting(1, GameRound::kTurn);
  EXPECT_EQ(actual_turn_oop.bet_sizes_percent, turn_oop_.bet_sizes_percent);
  EXPECT_EQ(actual_turn_oop.raise_sizes_percent, turn_oop_.raise_sizes_percent);
  EXPECT_EQ(actual_turn_oop.allow_all_in, turn_oop_.allow_all_in);

  const StreetSetting& actual_river_oop = settings_->GetSetting(1, GameRound::kRiver);
  EXPECT_EQ(actual_river_oop.bet_sizes_percent, river_oop_.bet_sizes_percent);
  EXPECT_EQ(actual_river_oop.raise_sizes_percent, river_oop_.raise_sizes_percent);
  EXPECT_EQ(actual_river_oop.allow_all_in, river_oop_.allow_all_in);
}

// Test GetSetting for invalid inputs
TEST_F(GameTreeBuildingSettingsTest, GetSettingInvalid) {
  ASSERT_NE(settings_, nullptr);

  // Invalid player index
  EXPECT_THROW(settings_->GetSetting(2, GameRound::kFlop), std::out_of_range);
  EXPECT_THROW(settings_->GetSetting(99, GameRound::kTurn), std::out_of_range);

  // Invalid round (out of range enum)
  EXPECT_THROW(settings_->GetSetting(0, static_cast<GameRound>(99)), std::logic_error);
  EXPECT_THROW(settings_->GetSetting(1, static_cast<GameRound>(99)), std::logic_error);
}

// Test default constructor (optional, if needed)
TEST(GameTreeBuildingSettingsDefaultTest, DefaultConstructor) {
    GameTreeBuildingSettings default_settings;
    // Default settings should be empty or have default StreetSetting values
    EXPECT_TRUE(default_settings.flop_ip_setting.bet_sizes_percent.empty());
    EXPECT_FALSE(default_settings.turn_oop_setting.allow_all_in);
}

TEST(GameTreeTest, RiverBettingRoundAfterTurnCheckCheck) {
    StreetSetting flop_ip{{50.0}, {60.0}, {}, false};
    StreetSetting turn_ip{{50.0}, {60.0}, {}, false};
    StreetSetting river_ip{{50.0}, {60.0}, {}, false};
    StreetSetting flop_oop{{50.0}, {60.0}, {}, false};
    StreetSetting turn_oop{{50.0}, {60.0}, {}, false};
    StreetSetting river_oop{{50.0}, {60.0}, {}, false};

    GameTreeBuildingSettings settings(flop_ip, turn_ip, river_ip, flop_oop, turn_oop, river_oop);
    Rule rule(
        Deck(),
        25.0,
        25.0,
        GameRound::kFlop,
        {43, 38, 2},
        3,
        1.0,
        2.0,
        200.0,
        settings,
        0.67
    );

    tree::GameTree tree(rule);
    NodeRef root = tree.GetRoot();
    ASSERT_EQ(root.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(root.index), 1); // OOP

    // Flop: OOP checks
    auto edges_flop_oop = tree.GetActionEdges(root.index);
    ASSERT_FALSE(edges_flop_oop.empty());
    NodeRef flop_ip_node = edges_flop_oop[0].child; // CHECK
    ASSERT_EQ(flop_ip_node.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(flop_ip_node.index), 0); // IP

    // Flop: IP checks
    auto edges_flop_ip = tree.GetActionEdges(flop_ip_node.index);
    ASSERT_FALSE(edges_flop_ip.empty());
    NodeRef turn_chance_node = edges_flop_ip[0].child; // CHECK
    ASSERT_EQ(turn_chance_node.type, GameTreeNodeType::kChance);
    ASSERT_EQ(tree.GetChanceRound(turn_chance_node.index), GameRound::kTurn);

    // Turn: Chance child deals Turn card -> Turn OOP Action Node
    NodeRef turn_oop_node = tree.GetChanceChild(turn_chance_node.index);
    ASSERT_EQ(turn_oop_node.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(turn_oop_node.index), 1); // OOP
    ASSERT_EQ(tree.GetActionRound(turn_oop_node.index), GameRound::kTurn);

    // Turn: OOP checks
    auto edges_turn_oop = tree.GetActionEdges(turn_oop_node.index);
    ASSERT_FALSE(edges_turn_oop.empty());
    NodeRef turn_ip_node = edges_turn_oop[0].child; // CHECK
    ASSERT_EQ(turn_ip_node.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(turn_ip_node.index), 0); // IP
    ASSERT_EQ(tree.GetActionRound(turn_ip_node.index), GameRound::kTurn);

    // Turn: IP checks -> Chance node for River deal
    auto edges_turn_ip = tree.GetActionEdges(turn_ip_node.index);
    ASSERT_FALSE(edges_turn_ip.empty());
    NodeRef river_chance_node = edges_turn_ip[0].child; // CHECK
    ASSERT_EQ(river_chance_node.type, GameTreeNodeType::kChance);
    ASSERT_EQ(tree.GetChanceRound(river_chance_node.index), GameRound::kRiver);

    // CRITICAL ASSERTION: After River card is dealt, there MUST be a River Action node!
    NodeRef river_oop_node = tree.GetChanceChild(river_chance_node.index);
    ASSERT_NE(river_oop_node.type, GameTreeNodeType::kShowdown)
        << "River deal went straight to Showdown instead of creating River betting round!";
    ASSERT_EQ(river_oop_node.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(river_oop_node.index), 1); // OOP acts first on River
    ASSERT_EQ(tree.GetActionRound(river_oop_node.index), GameRound::kRiver);

    auto edges_river_oop = tree.GetActionEdges(river_oop_node.index);
    ASSERT_GE(edges_river_oop.size(), 2); // At least Check and Bet on River!
}

TEST(GameTreeTest, RiverPayoffsAndActionsTest) {
    StreetSetting river_ip{{50.0}, {100.0}, {}, false};
    StreetSetting river_oop{{50.0}, {100.0}, {}, false};
    GameTreeBuildingSettings settings({}, {}, river_ip, {}, {}, river_oop);
    Rule rule(Deck(), 25.0, 25.0, GameRound::kRiver, {43, 38, 2, 8, 13}, 1, 1.0, 2.0, 200.0, settings, 0.67);
    tree::GameTree tree(rule);

    NodeRef root = tree.GetRoot();
    ASSERT_EQ(root.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(root.index), 1); // OOP acts first
    ASSERT_EQ(tree.GetActionPot(root.index), 50.0);

    auto root_edges = tree.GetActionEdges(root.index);
    ASSERT_GE(root_edges.size(), 2); // Check and Bet

    // 1. Check - Check -> Showdown
    ASSERT_EQ(root_edges[0].action.GetAction(), PokerAction::kCheck);
    NodeRef ip_after_check = root_edges[0].child;
    ASSERT_EQ(ip_after_check.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(ip_after_check.index), 0); // IP acts
    auto ip_edges = tree.GetActionEdges(ip_after_check.index);
    ASSERT_EQ(ip_edges[0].action.GetAction(), PokerAction::kCheck);
    NodeRef showdown_node = ip_edges[0].child;
    ASSERT_EQ(showdown_node.type, GameTreeNodeType::kShowdown);
    EXPECT_EQ(tree.GetShowdownPot(showdown_node.index), 50.0);
    auto showdown_payoffs = tree.GetShowdownPayoffs(showdown_node.index);
    EXPECT_DOUBLE_EQ(showdown_payoffs[0], 25.0);
    EXPECT_DOUBLE_EQ(showdown_payoffs[1], 25.0);

    // 2. Bet -> Fold / Call
    ASSERT_EQ(root_edges[1].action.GetAction(), PokerAction::kBet);
    double bet_val = root_edges[1].action.GetAmount();
    NodeRef ip_facing_bet = root_edges[1].child;
    ASSERT_EQ(ip_facing_bet.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(ip_facing_bet.index), 0);
    auto facing_bet_edges = tree.GetActionEdges(ip_facing_bet.index);

    bool found_call = false;
    bool found_fold = false;
    for (const auto& edge : facing_bet_edges) {
        if (edge.action.GetAction() == PokerAction::kCall) {
            found_call = true;
            ASSERT_EQ(edge.child.type, GameTreeNodeType::kShowdown);
            EXPECT_DOUBLE_EQ(tree.GetShowdownPot(edge.child.index), 50.0 + 2 * bet_val);
            auto call_payoffs = tree.GetShowdownPayoffs(edge.child.index);
            EXPECT_DOUBLE_EQ(call_payoffs[0], 25.0 + bet_val);
            EXPECT_DOUBLE_EQ(call_payoffs[1], 25.0 + bet_val);
        } else if (edge.action.GetAction() == PokerAction::kFold) {
            found_fold = true;
            ASSERT_EQ(edge.child.type, GameTreeNodeType::kTerminal);
            auto terminal_payoffs = tree.GetTerminalPayoffs(edge.child.index);
            // IP folded: IP loses 25, OOP wins 25
            EXPECT_DOUBLE_EQ(terminal_payoffs[0], -25.0);
            EXPECT_DOUBLE_EQ(terminal_payoffs[1], 25.0);
            EXPECT_DOUBLE_EQ(terminal_payoffs[0] + terminal_payoffs[1], 0.0);
        }
    }
    EXPECT_TRUE(found_call);
    EXPECT_TRUE(found_fold);
}

TEST(GameTreeTest, AllInRunoutDirectToShowdownTest) {
    // Stack = 30.0, commits = 25.0. Only 5.0 chips left!
    StreetSetting flop_ip{{100.0}, {}, {}, true};
    StreetSetting flop_oop{{100.0}, {}, {}, true};
    GameTreeBuildingSettings settings(flop_ip, {}, {}, flop_oop, {}, {});
    Rule rule(Deck(), 25.0, 25.0, GameRound::kFlop, {43, 38, 2}, 1, 1.0, 2.0, 30.0, settings, 0.67);
    tree::GameTree tree(rule);

    NodeRef root = tree.GetRoot();
    auto root_edges = tree.GetActionEdges(root.index);
    NodeRef ip_facing_shove = core::kNullNode;
    for (const auto& edge : root_edges) {
        if (edge.action.GetAction() == PokerAction::kBet) {
            ip_facing_shove = edge.child;
            break;
        }
    }
    ASSERT_NE(ip_facing_shove, core::kNullNode);
    auto ip_edges = tree.GetActionEdges(ip_facing_shove.index);
    NodeRef after_allin_call = core::kNullNode;
    for (const auto& edge : ip_edges) {
        if (edge.action.GetAction() == PokerAction::kCall) {
            after_allin_call = edge.child;
            break;
        }
    }
    ASSERT_NE(after_allin_call, core::kNullNode);
    // Because both players are now all-in, it should lead to Turn Chance node
    ASSERT_EQ(after_allin_call.type, GameTreeNodeType::kChance);
    ASSERT_EQ(tree.GetChanceRound(after_allin_call.index), GameRound::kTurn);

    // Turn Chance child should proceed DIRECTLY to River Chance node (no action node!)
    NodeRef turn_child = tree.GetChanceChild(after_allin_call.index);
    ASSERT_EQ(turn_child.type, GameTreeNodeType::kChance);
    ASSERT_EQ(tree.GetChanceRound(turn_child.index), GameRound::kRiver);

    // River Chance child should proceed DIRECTLY to Showdown (no action node!)
    NodeRef river_child = tree.GetChanceChild(turn_child.index);
    ASSERT_EQ(river_child.type, GameTreeNodeType::kShowdown);
    EXPECT_DOUBLE_EQ(tree.GetShowdownPot(river_child.index), 60.0); // 30 + 30
}

TEST(GameTreeTest, RaiseLimitPerStreetTest) {
    StreetSetting flop_ip{{50.0}, {60.0}, {}, false};
    StreetSetting flop_oop{{50.0}, {60.0}, {}, false};
    GameTreeBuildingSettings settings(flop_ip, {}, {}, flop_oop, {}, {});
    
    // Test 1: raise_limit_per_street = 1 -> OOP bet uses the 1 aggressive action allowed; IP cannot raise!
    {
        Rule rule(Deck(), 25.0, 25.0, GameRound::kFlop, {43, 38, 2}, 1, 1.0, 2.0, 500.0, settings, 0.67);
        tree::GameTree tree(rule);
        NodeRef root = tree.GetRoot();
        auto root_edges = tree.GetActionEdges(root.index);
        NodeRef ip_node = core::kNullNode;
        for (const auto& edge : root_edges) {
            if (edge.action.GetAction() == PokerAction::kBet) {
                ip_node = edge.child;
                break;
            }
        }
        ASSERT_NE(ip_node, core::kNullNode);
        auto ip_edges = tree.GetActionEdges(ip_node.index);
        for (const auto& edge : ip_edges) {
            EXPECT_NE(edge.action.GetAction(), PokerAction::kRaise)
                << "When raise_limit=1, IP should not be allowed to raise after OOP bet!";
        }
    }

    // Test 2: raise_limit_per_street = 2 -> OOP bet (1), IP raise (2), OOP cannot 3-bet!
    {
        Rule rule(Deck(), 25.0, 25.0, GameRound::kFlop, {43, 38, 2}, 2, 1.0, 2.0, 500.0, settings, 0.67);
        tree::GameTree tree(rule);
        NodeRef root = tree.GetRoot();
        auto root_edges = tree.GetActionEdges(root.index);
        NodeRef ip_node = core::kNullNode;
        for (const auto& edge : root_edges) {
            if (edge.action.GetAction() == PokerAction::kBet) {
                ip_node = edge.child;
                break;
            }
        }
        ASSERT_NE(ip_node, core::kNullNode);
        auto ip_edges = tree.GetActionEdges(ip_node.index);
        NodeRef oop_facing_raise = core::kNullNode;
        for (const auto& edge : ip_edges) {
            if (edge.action.GetAction() == PokerAction::kRaise) {
                oop_facing_raise = edge.child;
                break;
            }
        }
        ASSERT_NE(oop_facing_raise, core::kNullNode);
        auto oop_edges = tree.GetActionEdges(oop_facing_raise.index);
        for (const auto& edge : oop_edges) {
            EXPECT_NE(edge.action.GetAction(), PokerAction::kRaise)
                << "When raise_limit=2, OOP should not be allowed to re-raise after IP raise!";
            EXPECT_TRUE(edge.action.GetAction() == PokerAction::kCall ||
                        edge.action.GetAction() == PokerAction::kFold);
        }
    }
}

TEST(GameTreeTest, PreflopBBOptionAndTransitionsTest) {
    StreetSetting preflop_ip{{100.0}, {100.0}, {}, false};
    StreetSetting preflop_oop{{100.0}, {100.0}, {}, false};
    GameTreeBuildingSettings settings({}, {}, {}, {}, {}, {}, preflop_ip, preflop_oop);
    // OOP (BB) = 2.0, IP (SB) = 1.0
    Rule rule(Deck(), 2.0, 1.0, GameRound::kPreflop, {}, 2, 1.0, 2.0, 100.0, settings, 0.67);
    tree::GameTree tree(rule);

    NodeRef root = tree.GetRoot();
    ASSERT_EQ(root.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(root.index), 0); // Player 0 (SB) acts first preflop!
    ASSERT_EQ(tree.GetActionRound(root.index), GameRound::kPreflop);

    auto sb_edges = tree.GetActionEdges(root.index);
    NodeRef bb_node = core::kNullNode;
    for (const auto& edge : sb_edges) {
        if (edge.action.GetAction() == PokerAction::kCall) {
            bb_node = edge.child;
            break;
        }
    }
    ASSERT_NE(bb_node, core::kNullNode);
    ASSERT_EQ(bb_node.type, GameTreeNodeType::kAction);
    ASSERT_EQ(tree.GetActionPlayer(bb_node.index), 1); // BB acts!
    ASSERT_EQ(tree.GetActionRound(bb_node.index), GameRound::kPreflop);

    auto bb_edges = tree.GetActionEdges(bb_node.index);
    bool bb_has_check = false;
    for (const auto& edge : bb_edges) {
        if (edge.action.GetAction() == PokerAction::kCheck) {
            bb_has_check = true;
            ASSERT_EQ(edge.child.type, GameTreeNodeType::kChance);
            ASSERT_EQ(tree.GetChanceRound(edge.child.index), GameRound::kFlop);
        }
    }
    EXPECT_TRUE(bb_has_check);
}

TEST(GameTreeTest, GlobalZeroSumAndPotConservationTest) {
    StreetSetting street_setting{{50.0}, {60.0}, {}, true};
    GameTreeBuildingSettings settings(street_setting, street_setting, street_setting,
                                     street_setting, street_setting, street_setting);
    Rule rule(Deck(), 15.0, 15.0, GameRound::kRiver, {43, 38, 2, 8, 13}, 2, 1.0, 2.0, 100.0, settings, 0.67);
    tree::GameTree tree(rule);

    for (size_t i = 0; i < tree.GetNumTerminalNodes(); ++i) {
        auto payoffs = tree.GetTerminalPayoffs(i);
        EXPECT_NEAR(payoffs[0] + payoffs[1], 0.0, 1e-9)
            << "Terminal node " << i << " violates zero-sum property!";
    }

    for (size_t i = 0; i < tree.GetNumShowdownNodes(); ++i) {
        auto payoffs = tree.GetShowdownPayoffs(i);
        double pot = tree.GetShowdownPot(i);
        EXPECT_GT(pot, 0.0);
        EXPECT_NEAR(payoffs[0] + payoffs[1], pot, 1e-9)
            << "Showdown node " << i << " pot does not match commitments sum!";
    }
}

