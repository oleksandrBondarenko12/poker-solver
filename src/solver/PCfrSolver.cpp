#include "solver/PCfrSolver.h"
#include "Card.h"
#include "Library.h"
#include "tools/Rule.h"
#include "trainable/DiscountedCfrTrainable.h"
#include "trainable/Trainable.h"
#include "utils/Combinations.h"
#include "utils/FlopIsomorphism.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <omp.h>
#include <stdexcept>
#include <vector>

namespace poker_solver {
namespace solver {

namespace {
constexpr size_t kMaxHands = 1326;
constexpr size_t kMaxActions = 10;
} // namespace

PCfrSolver::PCfrSolver(std::shared_ptr<tree::GameTree> game_tree,
                       std::shared_ptr<ranges::PrivateCardsManager> pcm,
                       std::shared_ptr<ranges::RiverRangeManager> rrm,
                       const config::Rule &rule, Config solver_config)
    : Solver(std::move(game_tree)), pcm_(std::move(pcm)), rrm_(std::move(rrm)),
      deck_(rule.GetDeck()), config_(std::move(solver_config)),
      initial_board_mask_(
          core::Card::CardIntsToUint64(rule.GetInitialBoardCardsInt())) {}

void PCfrSolver::PreallocateTrainables(core::NodeRef node) {
  // Deprecated, allocating dynamically on the fly
}

void PCfrSolver::ExchangeColor(double *utility, size_t num_hands, int player,
                               int rank1, int rank2) const {
  if (rank1 == rank2)
    return;
  int self_ind[kMaxHands];
  int privateint2ind[52 * 52];
  std::fill_n(self_ind, num_hands, -1);
  std::fill_n(privateint2ind, 52 * 52, -1);
  const auto &range = pcm_->GetPlayerRange(player);
  for (size_t i = 0; i < num_hands; ++i) {
    int card1 = range[i].Card1Int();
    int card2 = range[i].Card2Int();
    if (card1 > card2)
      std::swap(card1, card2);
    self_ind[i] = card1 * 52 + card2;
    if (card1 % 4 == rank1)
      card1 = card1 - rank1 + rank2;
    else if (card1 % 4 == rank2)
      card1 = card1 - rank2 + rank1;
    if (card2 % 4 == rank1)
      card2 = card2 - rank1 + rank2;
    else if (card2 % 4 == rank2)
      card2 = card2 - rank2 + rank1;
    if (card1 > card2)
      std::swap(card1, card2);
    privateint2ind[card1 * 52 + card2] = static_cast<int>(i);
  }
  for (size_t i = 0; i < num_hands; ++i) {
    if (self_ind[i] == -1)
      continue;
    int ind = privateint2ind[self_ind[i]];
    if (ind != -1 && static_cast<size_t>(ind) != i) {
      self_ind[ind] = -1;
      std::swap(utility[i], utility[ind]);
    }
  }
}

void PCfrSolver::Train() {
  stop_signal_ = false;
  evs_calculated_ = false;
  if (config_.num_threads > 0)
    omp_set_num_threads(config_.num_threads);
  else
    omp_set_num_threads(1);

  std::vector<std::vector<double>> initial_reach_probs(num_players_);
  for (size_t p = 0; p < num_players_; ++p) {
    initial_reach_probs[p] = pcm_->GetInitialReachProbs(p);
  }

  PreallocateTrainables(game_tree_->GetRoot());

  for (int i = 1; i <= config_.iteration_limit; ++i) {
    if (stop_signal_)
      break;
    if (i % 100 == 0 && config_.progress_callback)
      config_.progress_callback(i);

    for (int traverser = 0; traverser < static_cast<int>(num_players_);
         ++traverser) {
      double root_utility[kMaxHands] = {0};
      cfr_utility(game_tree_->GetRoot(), initial_reach_probs[0].data(),
                  initial_reach_probs[1].data(), traverser, i,
                  initial_board_mask_, true, false, root_utility);
    }
  }
}

std::vector<double> PCfrSolver::ComputeEV() {
  std::vector<std::vector<double>> initial_reach_probs(num_players_);
  for (size_t p = 0; p < num_players_; ++p)
    initial_reach_probs[p] = pcm_->GetInitialReachProbs(p);

  std::vector<double> results(num_players_, 0.0);
  for (int traverser = 0; traverser < static_cast<int>(num_players_);
       ++traverser) {
    double evs[kMaxHands] = {0};
    cfr_utility(game_tree_->GetRoot(), initial_reach_probs[0].data(),
                initial_reach_probs[1].data(), traverser, 1,
                initial_board_mask_, true, true, evs);

    double unnormalized_ev = 0.0;
    for (size_t i = 0; i < pcm_->GetPlayerRange(traverser).size(); ++i) {
      unnormalized_ev += initial_reach_probs[traverser][i] * evs[i];
    }
    results[traverser] = unnormalized_ev;
  }
  evs_calculated_ = true;
  return results;
}

void PCfrSolver::Stop() { stop_signal_ = true; }

json PCfrSolver::DumpStrategy(bool dump_evs, int max_depth) const {
  // The root JSON is the root tree node itself
  json result =
      dump_strategy_recursive(game_tree_->GetRoot(), dump_evs, 0, max_depth);

  result["metadata"]["dump_evs"] = dump_evs;

  // Add ranges
  json ranges_array = json::array();
  for (size_t p = 0; p < num_players_; ++p) {
    json range_json = json::array();
    for (const auto &combo : pcm_->GetPlayerRange(p)) {
      range_json.push_back(combo.ToString());
    }
    ranges_array.push_back(range_json);
  }
  result["ranges"] = ranges_array;

  return result;
}

json PCfrSolver::dump_strategy_recursive(core::NodeRef node, bool dump_evs,
                                         int current_depth,
                                         int max_depth) const {
  if (node == core::kNullNode)
    return json::object();

  json result = json::object();

  if (node.type == core::GameTreeNodeType::kAction) {
    result["node_type"] = "Action";
    int player = game_tree_->GetActionPlayer(node.index);
    result["player"] = player;

    uint64_t initial_board = initial_board_mask_; // Use root board for top node
    uint64_t canonical_board =
        utils::FlopIsomorphism::GetCanonicalBoard(initial_board);

    auto edges = game_tree_->GetActionEdges(node.index);
    size_t num_actions = edges.size();
    size_t acting_player_num_hands = pcm_->GetPlayerRange(player).size();

    auto trainable = game_tree_->GetTrainable(
        node.index, canonical_board, num_actions, acting_player_num_hands);
    if (trainable) {
      result["strategy"] = trainable->DumpStrategy(dump_evs);
    }

    if (max_depth == -1 || current_depth < max_depth) {
      json children = json::array();
      for (const auto &edge : edges) {
        json child_obj = json::object();
        child_obj["action"] = edge.action.ToString();
        child_obj["node"] = dump_strategy_recursive(
            edge.child, dump_evs, current_depth + 1, max_depth);
        children.push_back(child_obj);
      }
      result["children"] = children;
    }
  } else if (node.type == core::GameTreeNodeType::kChance) {
    result["node_type"] = "Chance";
    // To avoid an infinite tree, we might not recurse deep into chance nodes if
    // not needed, but let's just dump it if depth allows.
    if (max_depth == -1 || current_depth < max_depth) {
      json children = json::array();
      json child_obj = json::object();
      child_obj["action"] = "chance";
      child_obj["node"] =
          dump_strategy_recursive(game_tree_->GetChanceChild(node.index),
                                  dump_evs, current_depth + 1, max_depth);
      children.push_back(child_obj);
      result["children"] = children;
    }
  } else if (node.type == core::GameTreeNodeType::kTerminal) {
    result["node_type"] = "Terminal";
  } else if (node.type == core::GameTreeNodeType::kShowdown) {
    result["node_type"] = "Showdown";
  }

  return result;
}

void PCfrSolver::cfr_utility(core::NodeRef node, const double *reach_probs_0,
                             const double *reach_probs_1, int traverser,
                             int iteration, uint64_t current_board_mask,
                             bool is_top_chance, bool use_average_strategy,
                             double *out_utility) {
  if (node == core::kNullNode)
    return;
  switch (node.type) {
  case core::GameTreeNodeType::kTerminal:
    cfr_terminal_node(node.index, reach_probs_0, reach_probs_1, traverser,
                      current_board_mask, out_utility);
    break;
  case core::GameTreeNodeType::kShowdown:
    cfr_showdown_node(node.index, reach_probs_0, reach_probs_1, traverser,
                      current_board_mask, out_utility);
    break;
  case core::GameTreeNodeType::kChance:
    cfr_chance_node(node.index, reach_probs_0, reach_probs_1, traverser,
                    iteration, current_board_mask, is_top_chance,
                    use_average_strategy, out_utility);
    break;
  case core::GameTreeNodeType::kAction:
    cfr_action_node(node.index, reach_probs_0, reach_probs_1, traverser,
                    iteration, current_board_mask, is_top_chance,
                    use_average_strategy, out_utility);
    break;
  }
}

void PCfrSolver::cfr_action_node(uint32_t node_idx, const double *reach_probs_0,
                                 const double *reach_probs_1, int traverser,
                                 int iteration, uint64_t current_board_mask,
                                 bool is_top_chance, bool use_average_strategy,
                                 double *out_utility) {

  size_t acting_player = game_tree_->GetActionPlayer(node_idx);
  auto edges = game_tree_->GetActionEdges(node_idx);
  size_t num_actions = edges.size();

  size_t traverser_num_hands = pcm_->GetPlayerRange(traverser).size();
  std::fill_n(out_utility, traverser_num_hands, 0.0);

  size_t acting_player_num_hands = pcm_->GetPlayerRange(acting_player).size();
  if (num_actions == 0 || acting_player_num_hands == 0)
    return;

  uint64_t canonical_board_mask =
      utils::FlopIsomorphism::GetCanonicalBoard(current_board_mask);
  auto trainable = game_tree_->GetTrainable(
      node_idx, canonical_board_mask, num_actions, acting_player_num_hands);
  const std::vector<double> &current_strategy_local =
      use_average_strategy ? trainable->GetAverageStrategy()
                           : trainable->GetCurrentStrategy();

  if (num_actions > kMaxActions) {
    throw std::runtime_error("Exceeded kMaxActions limit in cfr_action_node!");
  }

  double child_utilities[kMaxActions][kMaxHands] = {0};

  for (size_t a = 0; a < num_actions; ++a) {
    double next_reach_0[kMaxHands];
    double next_reach_1[kMaxHands];
    size_t hands_0 = pcm_->GetPlayerRange(0).size();
    size_t hands_1 = pcm_->GetPlayerRange(1).size();

    std::copy(reach_probs_0, reach_probs_0 + hands_0, next_reach_0);
    std::copy(reach_probs_1, reach_probs_1 + hands_1, next_reach_1);

    double *next_reach_acting =
        (acting_player == 0) ? next_reach_0 : next_reach_1;
    for (size_t h = 0; h < acting_player_num_hands; ++h) {
      size_t strat_idx = h * num_actions + a;
      next_reach_acting[h] *= current_strategy_local[strat_idx];
    }

    if (edges[a].child != core::kNullNode) {
      cfr_utility(edges[a].child, next_reach_0, next_reach_1, traverser,
                  iteration, current_board_mask, is_top_chance,
                  use_average_strategy, child_utilities[a]);
    }
  }

  if (acting_player == traverser) {
    for (size_t h = 0; h < traverser_num_hands; ++h) {
      for (size_t a = 0; a < num_actions; ++a) {
        out_utility[h] +=
            current_strategy_local[h * num_actions + a] * child_utilities[a][h];
      }
    }
  } else {
    for (size_t h = 0; h < traverser_num_hands; ++h) {
      for (size_t a = 0; a < num_actions; ++a) {
        out_utility[h] += child_utilities[a][h];
      }
    }
  }

  if (acting_player == traverser) {
    if (!use_average_strategy) {
      std::vector<double> weighted_regrets(num_actions *
                                           acting_player_num_hands);

      for (size_t h = 0; h < acting_player_num_hands; ++h) {
        for (size_t a = 0; a < num_actions; ++a) {
          weighted_regrets[h * num_actions + a] =
              child_utilities[a][h] - out_utility[h];
        }
      }

      trainable->UpdateRegrets(weighted_regrets, iteration);
      const double *actor_reach =
          (acting_player == 0) ? reach_probs_0 : reach_probs_1;
      const std::vector<double> &new_strategy = trainable->GetCurrentStrategy();
      trainable->AccumulateAverageStrategy(new_strategy, actor_reach,
                                           iteration);
    } else {
      std::vector<double> evs(num_actions * acting_player_num_hands);
      for (size_t h = 0; h < acting_player_num_hands; ++h) {
        for (size_t a = 0; a < num_actions; ++a) {
          evs[h * num_actions + a] = child_utilities[a][h];
        }
      }
      trainable->SetEv(evs);
    }
  }
}

void PCfrSolver::cfr_chance_node(uint32_t node_idx, const double *reach_probs_0,
                                 const double *reach_probs_1, int traverser,
                                 int iteration, uint64_t current_board_mask,
                                 bool is_top_chance, bool use_average_strategy,
                                 double *out_utility) {

  core::NodeRef child = game_tree_->GetChanceChild(node_idx);
  core::GameRound round_after_chance = game_tree_->GetChanceRound(node_idx);
  int num_cards_to_deal =
      (round_after_chance == core::GameRound::kFlop) ? 3 : 1;

  size_t traverser_num_hands = pcm_->GetPlayerRange(traverser).size();
  std::fill_n(out_utility, traverser_num_hands, 0.0);

  if (num_cards_to_deal == 3) {
    const auto &canonical_flops = utils::FlopIsomorphism::GetCanonicalFlops();

#pragma omp parallel if (is_top_chance)
    {
      double thread_utility[kMaxHands] = {0};

#pragma omp for schedule(dynamic) nowait
      for (size_t i = 0; i < canonical_flops.size(); ++i) {
        uint64_t outcome_board_mask = canonical_flops[i].first;
        double weight = canonical_flops[i].second;
        uint64_t next_board_mask = current_board_mask | outcome_board_mask;

        double next_reach_0[kMaxHands];
        double next_reach_1[kMaxHands];
        size_t hands_0 = pcm_->GetPlayerRange(0).size();
        size_t hands_1 = pcm_->GetPlayerRange(1).size();

        bool can_reach = false;
        for (size_t p = 0; p < num_players_; ++p) {
          const auto &range = pcm_->GetPlayerRange(p);
          const double *current_reach =
              (p == 0) ? reach_probs_0 : reach_probs_1;
          double *next_reach = (p == 0) ? next_reach_0 : next_reach_1;

          double current_reach_sum = 0.0;
          for (size_t h = 0; h < range.size(); ++h) {
            if (core::Card::DoBoardsOverlap(range[h].GetBoardMask(),
                                            outcome_board_mask)) {
              next_reach[h] = 0.0;
            } else {
              // Flop deals exactly 22100 combinations. The canonical branch
              // represents `weight` of them. But CFR reach probability requires
              // dividing by the total possible deals (22100).
              next_reach[h] = current_reach[h] / 22100.0;
            }
            current_reach_sum += next_reach[h];
          }
          if (current_reach_sum > 0.0)
            can_reach = true;
        }

        if (can_reach) {
          double child_ev[kMaxHands] = {0};
          cfr_utility(child, next_reach_0, next_reach_1, traverser, iteration,
                      next_board_mask, false, use_average_strategy, child_ev);

          for (size_t h = 0; h < traverser_num_hands; ++h) {
            thread_utility[h] += child_ev[h] * weight;
          }
        }
      }

#pragma omp critical
      {
        for (size_t u = 0; u < traverser_num_hands; ++u) {
          out_utility[u] += thread_utility[u];
        }
      }
    }
  } else {
    // 1-card deal logic
    std::vector<int> available_card_indices;
    available_card_indices.reserve(52);
    for (int i = 0; i < 52; ++i) {
      if (!core::Card::DoBoardsOverlap(1ULL << i, current_board_mask)) {
        available_card_indices.push_back(i);
      }
    }

    if (static_cast<int>(available_card_indices.size()) < num_cards_to_deal)
      return;

    utils::SimpleCombinations<int> combinations(available_card_indices,
                                                num_cards_to_deal);
    const auto &outcomes = combinations.GetCombinations();
    if (outcomes.empty())
      return;

    std::array<int, 4> current_iso_offset =
        utils::FlopIsomorphism::GetColorIsoOffset(current_board_mask);

    std::vector<int> valid_cards;
    valid_cards.reserve(outcomes.size());
    for (size_t i = 0; i < outcomes.size(); ++i) {
      int card_int = outcomes[i][0];
      int suit = card_int % 4;
      if (current_iso_offset[suit] >= 0) {
        valid_cards.push_back(i);
      }
    }

    std::vector<std::vector<double>> results(
        52, std::vector<double>(traverser_num_hands, 0.0));
    double possible_deals = outcomes.size();

#pragma omp parallel if (is_top_chance)
    {
#pragma omp for schedule(dynamic) nowait
      for (size_t valid_ind = 0; valid_ind < valid_cards.size(); ++valid_ind) {
        int i = valid_cards[valid_ind];
        int card_int = outcomes[i][0];
        uint64_t outcome_board_mask = core::Card::CardIntsToUint64(outcomes[i]);
        uint64_t next_board_mask = current_board_mask | outcome_board_mask;

        double next_reach_0[kMaxHands];
        double next_reach_1[kMaxHands];
        size_t hands_0 = pcm_->GetPlayerRange(0).size();
        size_t hands_1 = pcm_->GetPlayerRange(1).size();
        std::copy(reach_probs_0, reach_probs_0 + hands_0, next_reach_0);
        std::copy(reach_probs_1, reach_probs_1 + hands_1, next_reach_1);

        bool can_reach = false;
        for (size_t p = 0; p < num_players_; ++p) {
          const auto &range = pcm_->GetPlayerRange(p);
          double *next_reach = (p == 0) ? next_reach_0 : next_reach_1;
          double current_reach_sum = 0.0;
          for (size_t h = 0; h < range.size(); ++h) {
            if (core::Card::DoBoardsOverlap(range[h].GetBoardMask(),
                                            outcome_board_mask)) {
              next_reach[h] = 0.0;
            } else {
              next_reach[h] /= possible_deals;
            }
            current_reach_sum += next_reach[h];
          }
          if (current_reach_sum > 0.0)
            can_reach = true;
        }

        if (can_reach) {
          cfr_utility(child, next_reach_0, next_reach_1, traverser, iteration,
                      next_board_mask, false, use_average_strategy,
                      results[card_int].data());
        }
      }
    }

    for (size_t i = 0; i < outcomes.size(); ++i) {
      int card_int = outcomes[i][0];
      int suit = card_int % 4;
      int offset = current_iso_offset[suit];

      if (offset < 0) {
        int canonical_suit = suit + offset;
        int canonical_card = card_int + offset;

        std::vector<double> mapped_utility = results[canonical_card];
        ExchangeColor(mapped_utility.data(), traverser_num_hands, traverser,
                      canonical_suit, suit);

        for (size_t u = 0; u < traverser_num_hands; ++u) {
          out_utility[u] += mapped_utility[u];
        }
      } else {
        for (size_t u = 0; u < traverser_num_hands; ++u) {
          out_utility[u] += results[card_int][u];
        }
      }
    }
  }
}

void PCfrSolver::cfr_showdown_node(uint32_t node_idx,
                                   const double *reach_probs_0,
                                   const double *reach_probs_1, int traverser,
                                   uint64_t final_board_mask,
                                   double *out_utility) {

  int opponent_player = 1 - traverser;
  const auto &traverser_range = pcm_->GetPlayerRange(traverser);
  const auto &opponent_range = pcm_->GetPlayerRange(opponent_player);
  size_t traverser_hands = traverser_range.size();
  size_t opponent_hands = opponent_range.size();

  std::fill_n(out_utility, traverser_hands, 0.0);

  const auto &traverser_combos =
      rrm_->GetRiverCombos(traverser, traverser_range, final_board_mask);
  const auto &opponent_combos =
      rrm_->GetRiverCombos(opponent_player, opponent_range, final_board_mask);

  auto payoffs = game_tree_->GetShowdownPayoffs(node_idx);

  double total_oppo_mass = 0.0;
  double oppo_mass_by_card[52] = {0.0};

  const double *oppo_reach =
      (opponent_player == 0) ? reach_probs_0 : reach_probs_1;

  for (size_t oppo_orig_idx = 0; oppo_orig_idx < opponent_hands;
       ++oppo_orig_idx) {
    double mass = oppo_reach[oppo_orig_idx];
    total_oppo_mass += mass;

    int c1 = opponent_range[oppo_orig_idx].Card1Int();
    int c2 = opponent_range[oppo_orig_idx].Card2Int();
    oppo_mass_by_card[c1] += mass;
    oppo_mass_by_card[c2] += mass;
  }

  double win_mass = 0.0;
  double win_mass_by_card[52] = {0.0};

  size_t oppo_idx = 0;
  size_t tie_start_idx = 0;
  size_t tie_end_idx = 0;
  double tie_mass = 0.0;
  double tie_mass_by_card[52] = {0.0};

  for (size_t trav_idx = 0; trav_idx < traverser_combos.size(); ++trav_idx) {
    const auto &trav_c = traverser_combos[trav_idx];
    size_t trav_orig_idx = trav_c.original_range_index;

    while (oppo_idx < opponent_combos.size() &&
           opponent_combos[oppo_idx].rank > trav_c.rank) {
      size_t o_orig = opponent_combos[oppo_idx].original_range_index;
      double mass = oppo_reach[o_orig];
      win_mass += mass;
      int c1 = opponent_combos[oppo_idx].private_cards.Card1Int();
      int c2 = opponent_combos[oppo_idx].private_cards.Card2Int();
      win_mass_by_card[c1] += mass;
      win_mass_by_card[c2] += mass;
      oppo_idx++;
    }

    if (tie_start_idx != oppo_idx) {
      tie_start_idx = oppo_idx;
      tie_end_idx = oppo_idx;
      tie_mass = 0.0;
      std::fill_n(tie_mass_by_card, 52, 0.0);

      while (tie_end_idx < opponent_combos.size() &&
             opponent_combos[tie_end_idx].rank == trav_c.rank) {
        size_t o_orig = opponent_combos[tie_end_idx].original_range_index;
        double mass = oppo_reach[o_orig];
        tie_mass += mass;
        int c1 = opponent_combos[tie_end_idx].private_cards.Card1Int();
        int c2 = opponent_combos[tie_end_idx].private_cards.Card2Int();
        tie_mass_by_card[c1] += mass;
        tie_mass_by_card[c2] += mass;
        tie_end_idx++;
      }
    }

    double loss_mass = total_oppo_mass - win_mass - tie_mass;

    int t1 = trav_c.private_cards.Card1Int();
    int t2 = trav_c.private_cards.Card2Int();

    double win_blockers = win_mass_by_card[t1] + win_mass_by_card[t2];
    double tie_blockers = tie_mass_by_card[t1] + tie_mass_by_card[t2];
    double total_blockers = oppo_mass_by_card[t1] + oppo_mass_by_card[t2];

    auto oppo_idx_opt =
        pcm_->GetOpponentHandIndex(traverser, opponent_player, trav_orig_idx);
    double self_mass = 0.0;
    if (oppo_idx_opt) {
      self_mass = oppo_reach[*oppo_idx_opt];
    }

    tie_blockers -= self_mass;
    total_blockers -= self_mass;

    double loss_blockers = total_blockers - win_blockers - tie_blockers;

    double actual_win_mass = win_mass - win_blockers;
    double actual_tie_mass = tie_mass - tie_blockers;
    double actual_loss_mass = loss_mass - loss_blockers;

    double expected_value_for_hand = actual_win_mass * payoffs[1 - traverser] +
                                     actual_tie_mass * 0.0 +
                                     actual_loss_mass * (-payoffs[traverser]);

    out_utility[trav_orig_idx] = expected_value_for_hand;
  }
}

void PCfrSolver::cfr_terminal_node(uint32_t node_idx,
                                   const double *reach_probs_0,
                                   const double *reach_probs_1, int traverser,
                                   uint64_t current_board_mask,
                                   double *out_utility) {

  auto payoffs = game_tree_->GetTerminalPayoffs(node_idx);
  double payoff_for_traverser = payoffs[traverser];

  int opponent_player = 1 - traverser;
  const auto &traverser_range = pcm_->GetPlayerRange(traverser);
  const auto &opponent_range = pcm_->GetPlayerRange(opponent_player);
  size_t traverser_hands = traverser_range.size();
  size_t opponent_hands = opponent_range.size();

  std::fill_n(out_utility, traverser_hands, 0.0);
  const double *oppo_reach =
      (opponent_player == 0) ? reach_probs_0 : reach_probs_1;

  double oppo_prob_sum = 0.0;
  double oppo_card_sum[52] = {0.0};

  for (size_t h_j = 0; h_j < opponent_hands; ++h_j) {
    uint64_t opponent_mask = opponent_range[h_j].GetBoardMask();
    if (core::Card::DoBoardsOverlap(opponent_mask, current_board_mask))
      continue;

    double p = oppo_reach[h_j];
    oppo_prob_sum += p;
    oppo_card_sum[opponent_range[h_j].Card1Int()] += p;
    oppo_card_sum[opponent_range[h_j].Card2Int()] += p;
  }

  for (size_t h_i = 0; h_i < traverser_hands; ++h_i) {
    uint64_t traverser_mask = traverser_range[h_i].GetBoardMask();
    if (core::Card::DoBoardsOverlap(traverser_mask, current_board_mask)) {
      continue;
    }

    int t1 = traverser_range[h_i].Card1Int();
    int t2 = traverser_range[h_i].Card2Int();

    auto oppo_idx_opt =
        pcm_->GetOpponentHandIndex(traverser, opponent_player, h_i);
    double self_mass = 0.0;
    if (oppo_idx_opt) {
      self_mass = oppo_reach[*oppo_idx_opt];
    }

    double compatible_opponent_reach_sum =
        oppo_prob_sum - oppo_card_sum[t1] - oppo_card_sum[t2] + self_mass;
    out_utility[h_i] = payoff_for_traverser * compatible_opponent_reach_sum;
  }
}

} // namespace solver
} // namespace poker_solver
