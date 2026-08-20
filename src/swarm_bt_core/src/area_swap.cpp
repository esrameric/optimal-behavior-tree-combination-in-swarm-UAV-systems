#include "swarm_bt_core/area_swap.hpp"

#include <algorithm>
#include <cmath>
#include <map>
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

  // Devir birimi TAM SUTUNDUR, tek tek hucre degil.
  //
  // Gerekce (olculdu): bicerdover taramada tek tuk hucre devretmek aliciyi yeni
  // bir sutuna sapmaya zorlarken teklif edenin zaten gececegi yoldan neredeyse
  // hicbir sey eksiltmez -- ortalama fayda -333 m cikiyor ve HICBIR takas kabul
  // edilmiyordu. Tam sutun devredildiginde teklif eden o sutuna hic ugramaz;
  // kazanc gercek olur ve fayda olcutu anlamli bir karar verebilir. Ayrica
  // P3 tahsis algoritmalari da sutun granulerliginde calisir.
  const auto & area = state.area();
  std::map<int, std::vector<int>> proposer_columns;
  for (const int cell_id : state.remainingCellIds(proposer)) {
    proposer_columns[area.colOf(cell_id)].push_back(cell_id);
  }
  if (proposer_columns.empty()) {
    return std::nullopt;
  }

  // Sutunlari aliciya yakinliga gore sirala.
  const Vec2 receiver_position = state.agent(receiver).position;
  std::vector<int> columns;
  columns.reserve(proposer_columns.size());
  for (const auto & entry : proposer_columns) {
    columns.push_back(entry.first);
  }
  std::sort(
    columns.begin(), columns.end(),
    [&area, &receiver_position](int lhs, int rhs) {
      const double lhs_x = (lhs + 0.5) * area.cellSize();
      const double rhs_x = (rhs + 0.5) * area.cellSize();
      return std::abs(receiver_position.x - lhs_x) < std::abs(receiver_position.x - rhs_x);
    });

  // Devredilecek sutun sayisi: ikiliyi dengeleyecek kadar.
  const int difference = state.remainingCells(proposer) - state.remainingCells(receiver);
  const int rows = std::max(1, area.rows());
  const int transfer_columns = std::min(
    static_cast<int>(columns.size()), std::max(1, difference / (2 * rows)));

  SwapProposal proposal;
  proposal.proposer_id = proposer;
  proposal.receiver_id = receiver;
  for (int i = 0; i < transfer_columns; ++i) {
    const auto & cells = proposer_columns[columns[static_cast<std::size_t>(i)]];
    proposal.offered_cells.insert(proposal.offered_cells.end(), cells.begin(), cells.end());
  }
  if (proposal.offered_cells.empty()) {
    return std::nullopt;
  }

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

std::vector<int> AreaSwapNegotiator::boundaryCells(
  const SwarmState & state, int agent_a, int agent_b)
{
  const auto & area = state.area();
  const auto & region_b = state.agent(agent_b).region;
  const std::unordered_set<int> cells_b(region_b.begin(), region_b.end());

  std::unordered_set<int> boundary;
  for (const int cell_id : state.agent(agent_a).region) {
    const int col = area.colOf(cell_id);
    const int row = area.rowOf(cell_id);
    const int neighbours[4][2] = {{col - 1, row}, {col + 1, row}, {col, row - 1}, {col, row + 1}};
    for (const auto & neighbour : neighbours) {
      if (neighbour[0] < 0 || neighbour[0] >= area.cols() ||
        neighbour[1] < 0 || neighbour[1] >= area.rows())
      {
        continue;
      }
      const int neighbour_id = area.cellId(neighbour[0], neighbour[1]);
      if (cells_b.count(neighbour_id) > 0) {
        boundary.insert(cell_id);
        boundary.insert(neighbour_id);
      }
    }
  }

  std::vector<int> result(boundary.begin(), boundary.end());
  std::sort(result.begin(), result.end());
  return result;
}

double AreaSwapNegotiator::boundaryPheromone(
  const SwarmState & state, int agent_a, int agent_b)
{
  return state.interest().meanOver(boundaryCells(state, agent_a, agent_b));
}

int AreaSwapNegotiator::startJointScan(SwarmState * state, int agent_a, int agent_b)
{
  if (state == nullptr) {
    throw std::invalid_argument("AreaSwapNegotiator::startJointScan: durum bos olamaz");
  }

  std::vector<int> shared;
  for (const int cell_id : boundaryCells(*state, agent_a, agent_b)) {
    if (!state->isVisited(cell_id)) {
      shared.push_back(cell_id);
    }
  }
  if (shared.empty()) {
    return 0;
  }

  int added = 0;
  for (const int agent_id : {agent_a, agent_b}) {
    auto & region = state->agent(agent_id).region;
    const std::unordered_set<int> owned(region.begin(), region.end());
    for (const int cell_id : shared) {
      if (owned.count(cell_id) == 0) {
        region.push_back(cell_id);
        ++added;
      }
    }
    state->resequenceRegion(agent_id);
  }

  // Ortak tarama da bir atama degisikligidir: iki ajanin da bolgesi buyudu.
  const int shared_per_agent = static_cast<int>(shared.size());
  state->recordAssignmentChange(
    agent_a, AssignmentReason::kJointScan, agent_b, shared_per_agent);
  state->recordAssignmentChange(
    agent_b, AssignmentReason::kJointScan, agent_a, shared_per_agent);
  return added;
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

  // Bolum 6: her iki ajanin da atanmis alani degisti; olay kaydi duşuyor.
  const int transferred = static_cast<int>(proposal.offered_cells.size());
  state->recordAssignmentChange(
    proposal.proposer_id, AssignmentReason::kAreaSwap, proposal.receiver_id, transferred);
  state->recordAssignmentChange(
    proposal.receiver_id, AssignmentReason::kAreaSwap, proposal.proposer_id, transferred);
}

}  // namespace swarm_bt_core
