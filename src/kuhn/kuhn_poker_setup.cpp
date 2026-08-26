#include <cstdint>
#include "kuhn/kuhn_poker_setup.h"
#include "nodes/GameActions.h" // For core::PokerAction
#include "ranges/PrivateCards.h" // For core::PrivateCards
#include "tools/Rule.h"
#include "tools/GameTreeBuildingSettings.h"
#include "Deck.h"

#include <vector>
#include <memory>

namespace poker_solver {
namespace kuhn {

// --- KuhnCompairer Implementation ---

core::ComparisonResult KuhnCompairer::CompareHands(int card1, int card2) const {
    if (card1 > card2) return core::ComparisonResult::kPlayer1Wins; // P0 wins (in typical CompareHands, P1 is first arg)
    if (card2 > card1) return core::ComparisonResult::kPlayer2Wins; // P1 wins
    return core::ComparisonResult::kTie;
}

core::ComparisonResult KuhnCompairer::CompareHands(
    const std::vector<int>& private_hand1,
    const std::vector<int>& private_hand2,
    const std::vector<int>&) const {
    return CompareHands(private_hand1[0], private_hand2[0]);
}

core::ComparisonResult KuhnCompairer::CompareHands(uint64_t mask1, uint64_t mask2, uint64_t) const {
    auto hand1 = core::Card::Uint64ToCardInts(mask1);
    auto hand2 = core::Card::Uint64ToCardInts(mask2);
    // Find the non-dummy card (which is < 50)
    int c1 = (hand1[0] < 50) ? hand1[0] : hand1[1];
    int c2 = (hand2[0] < 50) ? hand2[0] : hand2[1];
    return CompareHands(c1, c2);
}

int KuhnCompairer::GetHandRank(const std::vector<int>& private_hand,
                               const std::vector<int>&) const {
    return private_hand[0];
}

int KuhnCompairer::GetHandRank(uint64_t mask, uint64_t) const {
    auto hand = core::Card::Uint64ToCardInts(mask);
    int c1 = (hand[0] < 50) ? hand[0] : hand[1];
    return c1;
}


// --- Kuhn Game Tree Builder Implementation ---

std::shared_ptr<tree::GameTree> build_kuhn_game_tree() {
    core::Deck dummy_deck;
    auto tree = std::make_shared<tree::GameTree>(dummy_deck);

    // Terminal payoffs: {P0_net_win, P1_net_win}
    // Showdown payoffs: {P0_commit, P1_commit}

    // Nodes
    // 1. Terminal: P0 Check, P1 Bet, P0 Fold -> P1 wins pot of 2 (ante 1 each). P0 loses 1, P1 wins 1.
    auto term_p0_fold = tree->CreateTerminalNode(2.0, {-1.0, 1.0});

    // 2. Terminal: P0 Bet, P1 Fold -> P0 wins pot of 2 (ante 1 each). P0 wins 1, P1 loses 1.
    auto term_p1_fold = tree->CreateTerminalNode(2.0, {1.0, -1.0});

    // 3. Showdown: P0 Check, P1 Check -> Pot is 2 (ante 1 each).
    auto show_chk_chk = tree->CreateShowdownNode(core::GameRound::kRiver, 2.0, {1.0, 1.0});

    // 4. Showdown: P0 Check, P1 Bet, P0 Call -> Pot is 4 (ante 1 + bet 1 each).
    auto show_bet_call = tree->CreateShowdownNode(core::GameRound::kRiver, 4.0, {2.0, 2.0});

    // 5. Showdown: P0 Bet, P1 Call -> Pot is 4 (ante 1 + bet 1 each).
    // Note: identical to #4
    auto show_bet_call_2 = tree->CreateShowdownNode(core::GameRound::kRiver, 4.0, {2.0, 2.0});

    // Action Nodes
    // P1 facing P0 Check (Pot = 2)
    auto p1_facing_chk = tree->CreateActionNode(1, core::GameRound::kPreflop, 2.0);
    tree->BuildActionNode(p1_facing_chk.index, 
                          {core::GameAction(core::PokerAction::kCheck), core::GameAction(core::PokerAction::kBet, 1.0)}, 
                          {show_chk_chk, tree->CreateActionNode(0, core::GameRound::kPreflop, 2.0)});

    // Wait, the child of P1 Bet is P0 facing P1 Bet.
    auto p0_facing_bet = tree->GetActionEdges(p1_facing_chk.index)[1].child;
    tree->BuildActionNode(p0_facing_bet.index, 
                          {core::GameAction(core::PokerAction::kFold), core::GameAction(core::PokerAction::kCall)}, 
                          {term_p0_fold, show_bet_call});

    // P1 facing P0 Bet (Pot = 2)
    auto p1_facing_bet = tree->CreateActionNode(1, core::GameRound::kPreflop, 2.0);
    tree->BuildActionNode(p1_facing_bet.index, 
                          {core::GameAction(core::PokerAction::kFold), core::GameAction(core::PokerAction::kCall)}, 
                          {term_p1_fold, show_bet_call_2});

    // Root node: P0 acts first (Pot = 2)
    auto root = tree->CreateActionNode(0, core::GameRound::kPreflop, 2.0);
    tree->BuildActionNode(root.index, 
                          {core::GameAction(core::PokerAction::kCheck), core::GameAction(core::PokerAction::kBet, 1.0)}, 
                          {p1_facing_chk, p1_facing_bet});

    tree->SetRoot(root);
    tree->CalculateTreeMetadata();
    return tree;
}

// --- Helper: Kuhn Range ---

std::vector<core::PrivateCards> get_kuhn_initial_range(int dummy_card) {
    std::vector<core::PrivateCards> range;
    for (int card = KUHN_CARD_J; card <= KUHN_CARD_K; ++card) {
        // We only use card1 for Kuhn. We set card2 to a non-overlapping dummy card.
        range.emplace_back(card, dummy_card, 1.0);
    }
    return range;
}

} // namespace kuhn
} // namespace poker_solver
