#include "swarm_bt_core/area_swap.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace swarm_bt_core
{

namespace
{

/// Baslangic konumundan itibaren hucre merkezlerini sirayla gezme mesafesi.
double pathLength(const MissionArea & area, const Vec2 & start, const std::vector<int> & cells)
{
  if (cells.empty()) {
    return 0.0;
  }
  double total = distance(start, area.cellCenter(cells.front()));
  for (std::size_t i = 1; i < cells.size(); ++i) {
    total += distance(area.cellCenter(cells[i - 1]), area.cellCenter(cells[i]));
  }
  return total;
}

/// Hucre kumesini ajanin kendi supurme yonuyle tarama sirasina dizer.
std::vector<int> sequencedFor(
  const SwarmState & state, int agent_id, std::vector<int> cells)
{
  return state.area().boustrophedonOrder(std::move(cells), state.agent(agent_id).sweep_reversed);
}

}  // namespace

AreaSwapNegotiator::AreaSwapNegotiator(double swap_threshold)
: swap_threshold_(swap_threshold)
{
  if (swap_threshold < 0.0 || swap_threshold > 1.0) {
    throw std::invalid_argument("AreaSwapNegotiator: esik_degeri [0,1] araliginda olmali");
  }
}

double AreaSwapNegotiator::imbalance(const SwarmState & state, int agent_a, int agent_b)
{
  return std::abs(state.remainingRatio(agent_a) - state.remainingRatio(agent_b));
}

bool AreaSwapNegotiator::imbalanceAboveThreshold(
  const SwarmState & state, int agent_a, int agent_b) const
{
  return imbalance(state, agent_a, agent_b) > swap_threshold_;
}

double AreaSwapNegotiator::tourLength(const SwarmState & state, int agent_id)
{
  return pathLength(
    state.area(), state.agent(agent_id).position, state.remainingCellIds(agent_id));
}

double AreaSwapNegotiator::tourLengthWith(
  const SwarmState & state, int agent_id, const std::vector<int> & extra_cells)
{
  auto cells = state.remainingCellIds(agent_id);
  for (const int cell_id : extra_cells) {
    if (!state.isVisited(cell_id)) {
      cells.push_back(cell_id);
    }
  }
  return pathLength(
    state.area(), state.agent(agent_id).position, sequencedFor(state, agent_id, cells));
}

double AreaSwapNegotiator::tourLengthWithout(
  const SwarmState & state, int agent_id, const std::vector<int> & removed_cells)
{
  const std::unordered_set<int> removed(removed_cells.begin(), removed_cells.end());
  std::vector<int> cells;
  for (const int cell_id : state.remainingCellIds(agent_id)) {
    if (removed.count(cell_id) == 0) {
      cells.push_back(cell_id);
    }
  }
  return pathLength(state.area(), state.agent(agent_id).position, cells);
}

std::optional<SwapProposal> AreaSwapNegotiator::buildProposal(
  const SwarmState & state, int agent_a, int agent_b) const
{
  if (agent_a == agent_b) {
    throw std::invalid_argument("AreaSwapNegotiator: bir ajan kendisiyle takas yapamaz");
  }
  if (!imbalanceAboveThreshold(state, agent_a, agent_b)) {
    return std::nullopt;
  }

  // Kalan alani FAZLA olan teklif eder.
  const bool a_has_more = state.remainingRatio(agent_a) > state.remainingRatio(agent_b);
  const int proposer = a_has_more ? agent_a : agent_b;
  const int receiver = a_has_more ? agent_b : agent_a;

  auto proposer_cells = state.remainingCellIds(proposer);
  if (proposer_cells.empty()) {
    return std::nullopt;
  }

  // Ikiliyi dengeleyecek hucre sayisi: farkin yarisi.
  const int difference = state.remainingCells(proposer) - state.remainingCells(receiver);
  const int transfer_count =
    std::min(static_cast<int>(proposer_cells.size()), std::max(1, difference / 2));

  // Aliciya EN YAKIN hucreler devredilir: takasin mesafe kazanci buradan gelir.
  const Vec2 receiver_position = state.agent(receiver).position;
  std::partial_sort(
    proposer_cells.begin(), proposer_cells.begin() + transfer_count, proposer_cells.end(),
    [&state, &receiver_position](int lhs, int rhs) {
      return distance(receiver_position, state.area().cellCenter(lhs)) <
      distance(receiver_position, state.area().cellCenter(rhs));
    });

  SwapProposal proposal;
  proposal.proposer_id = proposer;
  proposal.receiver_id = receiver;
  proposal.offered_cells.assign(
    proposer_cells.begin(), proposer_cells.begin() + transfer_count);

  proposal.proposer_gain =
    tourLength(state, proposer) - tourLengthWithout(state, proposer, proposal.offered_cells);
  proposal.receiver_cost =
    tourLengthWith(state, receiver, proposal.offered_cells) - tourLength(state, receiver);

  return proposal;
}

int AreaSwapNegotiator::distributeOrphans(SwarmState * state, int agent_a, int agent_b)
{
  if (state == nullptr) {
    throw std::invalid_argument("AreaSwapNegotiator::distributeOrphans: durum bos olamaz");
  }
  const auto orphans = state->orphanedCells();
  if (orphans.empty()) {
    return 0;
  }

  const bool a_alive = state->agent(agent_a).alive;
  const bool b_alive = state->agent(agent_b).alive;
  if (!a_alive && !b_alive) {
    return 0;
  }

  std::vector<int> for_a;
  std::vector<int> for_b;
  const Vec2 position_a = state->agent(agent_a).position;
  const Vec2 position_b = state->agent(agent_b).position;

  for (const int cell_id : orphans) {
    if (state->isVisited(cell_id)) {
      continue;
    }
    if (!b_alive) {
      for_a.push_back(cell_id);
      continue;
    }
    if (!a_alive) {
      for_b.push_back(cell_id);
      continue;
    }
    const Vec2 center = state->area().cellCenter(cell_id);
    if (distance(position_a, center) <= distance(position_b, center)) {
      for_a.push_back(cell_id);
    } else {
      for_b.push_back(cell_id);
    }
  }

  state->claimOrphanedCells(agent_a, for_a);
  state->claimOrphanedCells(agent_b, for_b);
  return static_cast<int>(for_a.size() + for_b.size());
}

int AreaSwapNegotiator::claimOrphansIfIdle(SwarmState * state, int agent_id)
{
  if (state == nullptr) {
    throw std::invalid_argument("AreaSwapNegotiator::claimOrphansIfIdle: durum bos olamaz");
  }
  const auto & agent = state->agent(agent_id);
  if (!agent.alive || state->orphanedCells().empty() || state->remainingCells(agent_id) > 0) {
    return 0;
  }

  std::vector<int> claimable;
  for (const int cell_id : state->orphanedCells()) {
    if (!state->isVisited(cell_id)) {
      claimable.push_back(cell_id);
    }
  }
  if (claimable.empty()) {
    return 0;
  }

  state->claimOrphanedCells(agent_id, claimable);
  return static_cast<int>(claimable.size());
}

void AreaSwapNegotiator::apply(SwarmState * state, const SwapProposal & proposal) const
{
  if (state == nullptr) {
    throw std::invalid_argument("AreaSwapNegotiator::apply: durum bos olamaz");
  }
  if (proposal.offered_cells.empty()) {
    return;
  }

  const std::unordered_set<int> offered(
    proposal.offered_cells.begin(), proposal.offered_cells.end());

  auto & proposer = state->agent(proposal.proposer_id);
  std::vector<int> kept;
  kept.reserve(proposer.region.size());
  for (const int cell_id : proposer.region) {
    if (offered.count(cell_id) == 0) {
      kept.push_back(cell_id);
    }
  }
  proposer.region = std::move(kept);

  auto & receiver = state->agent(proposal.receiver_id);
  receiver.region.insert(
    receiver.region.end(), proposal.offered_cells.begin(), proposal.offered_cells.end());

  // Iki tarafin da rotasi yeniden planlanir.
  state->resequenceRegion(proposal.proposer_id);
  state->resequenceRegion(proposal.receiver_id);

  // Bolum 6 metrigi (atama kararliligi): her iki ajanin da atanmis alani degisti.
  ++state->agent(proposal.proposer_id).assignment_changes;
  ++state->agent(proposal.receiver_id).assignment_changes;
}

}  // namespace swarm_bt_core
