#include "swarm_bt_core/swarm_state.hpp"

#include <cmath>
#include <random>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "swarm_bt_core/region_allocator.hpp"

namespace swarm_bt_core
{

SwarmState::SwarmState(const MissionArea & area, int agent_count, double pheromone_decay)
: area_(area),
  visited_(area.cellCount(), 0.0),
  interest_(area.cellCount(), pheromone_decay)
{
  if (agent_count <= 0) {
    throw std::invalid_argument("SwarmState: ajan sayisi pozitif olmali");
  }
  agents_.resize(static_cast<std::size_t>(agent_count));
  for (int i = 0; i < agent_count; ++i) {
    agents_[static_cast<std::size_t>(i)].id = i;
    agents_[static_cast<std::size_t>(i)].known_visited.assign(
      static_cast<std::size_t>(area.cellCount()), 0u);
  }
}

AgentState & SwarmState::agent(int id)
{
  if (id < 0 || id >= agentCount()) {
    throw std::out_of_range("SwarmState::agent: gecersiz ajan kimligi");
  }
  return agents_[static_cast<std::size_t>(id)];
}

const AgentState & SwarmState::agent(int id) const
{
  if (id < 0 || id >= agentCount()) {
    throw std::out_of_range("SwarmState::agent: gecersiz ajan kimligi");
  }
  return agents_[static_cast<std::size_t>(id)];
}

bool SwarmState::isVisited(int cell_id) const
{
  return visited_.at(cell_id) > PheromoneGrid::kEpsilon;
}

void SwarmState::markVisited(int cell_id)
{
  visited_.set(cell_id, 1.0);
}

bool SwarmState::knowsVisited(int agent_id, int cell_id) const
{
  if (stigmergy_) {
    return isVisited(cell_id);
  }
  const auto & known = agent(agent_id).known_visited;
  if (cell_id < 0 || cell_id >= static_cast<int>(known.size())) {
    throw std::out_of_range("SwarmState::knowsVisited: gecersiz hucre kimligi");
  }
  return known[static_cast<std::size_t>(cell_id)] != 0u;
}

void SwarmState::markVisitedBy(int agent_id, int cell_id)
{
  markVisited(cell_id);
  auto & known = agent(agent_id).known_visited;
  if (cell_id < 0 || cell_id >= static_cast<int>(known.size())) {
    throw std::out_of_range("SwarmState::markVisitedBy: gecersiz hucre kimligi");
  }
  known[static_cast<std::size_t>(cell_id)] = 1u;
}

int SwarmState::shareKnowledge(int agent_a, int agent_b)
{
  auto & known_a = agent(agent_a).known_visited;
  auto & known_b = agent(agent_b).known_visited;
  int learned = 0;
  for (std::size_t i = 0; i < known_a.size(); ++i) {
    if (known_a[i] != known_b[i]) {
      ++learned;
      known_a[i] = 1u;
      known_b[i] = 1u;
    }
  }
  return learned;
}

int SwarmState::remainingCellsKnown(int agent_id) const
{
  int remaining = 0;
  for (const int cell_id : agent(agent_id).region) {
    if (!knowsVisited(agent_id, cell_id)) {
      ++remaining;
    }
  }
  return remaining;
}

int SwarmState::remainingCells(int agent_id) const
{
  int remaining = 0;
  for (const int cell_id : agent(agent_id).region) {
    if (!isVisited(cell_id)) {
      ++remaining;
    }
  }
  return remaining;
}

std::vector<int> SwarmState::remainingCellIds(int agent_id) const
{
  std::vector<int> remaining;
  for (const int cell_id : agent(agent_id).region) {
    if (!isVisited(cell_id)) {
      remaining.push_back(cell_id);
    }
  }
  return remaining;
}

void SwarmState::resequenceRegion(int agent_id)
{
  auto & a = agent(agent_id);
  a.region = area_.boustrophedonOrder(a.region, a.sweep_reversed);
  // Ziyaret edilmis hucreler nextTargetCell() tarafindan atlandigi icin
  // basa donmek guvenli ve yeniden planlamanin dogru davranisi.
  a.next_waypoint = 0;
}

double SwarmState::remainingRatio(int agent_id) const
{
  const auto & region = agent(agent_id).region;
  if (region.empty()) {
    return 0.0;
  }
  return static_cast<double>(remainingCells(agent_id)) / static_cast<double>(region.size());
}

void SwarmState::failAgent(int agent_id)
{
  auto & a = agent(agent_id);
  if (!a.alive) {
    return;
  }
  a.alive = false;

  // Taranmamis hucreler sahipsiz havuza; taranmis olanlar zaten kapatildi.
  for (const int cell_id : a.region) {
    if (!isVisited(cell_id)) {
      orphaned_cells_.push_back(cell_id);
    }
  }
  a.region.clear();
  a.next_waypoint = 0;
  ++a.assignment_changes;
}

void SwarmState::claimOrphanedCells(int agent_id, const std::vector<int> & cells)
{
  if (cells.empty()) {
    return;
  }
  auto & a = agent(agent_id);
  if (!a.alive) {
    throw std::invalid_argument("SwarmState::claimOrphanedCells: arizali ajan devralamaz");
  }

  const std::unordered_set<int> claimed(cells.begin(), cells.end());
  std::vector<int> remaining_orphans;
  int transferred = 0;
  for (const int cell_id : orphaned_cells_) {
    if (claimed.count(cell_id) > 0) {
      a.region.push_back(cell_id);
      ++transferred;
    } else {
      remaining_orphans.push_back(cell_id);
    }
  }
  orphaned_cells_ = std::move(remaining_orphans);

  if (transferred > 0) {
    resequenceRegion(agent_id);
    ++a.assignment_changes;
  }
}

bool SwarmState::coverageComplete() const
{
  for (const int cell_id : orphaned_cells_) {
    if (!isVisited(cell_id)) {
      return false;
    }
  }
  for (const auto & a : agents_) {
    for (const int cell_id : a.region) {
      if (!isVisited(cell_id)) {
        return false;
      }
    }
  }
  return true;
}

double SwarmState::coverageImbalance() const
{
  if (agents_.empty()) {
    return 0.0;
  }
  double sum = 0.0;
  for (const auto & a : agents_) {
    sum += static_cast<double>(remainingCells(a.id));
  }
  const double mean = sum / static_cast<double>(agents_.size());

  double variance = 0.0;
  for (const auto & a : agents_) {
    const double diff = static_cast<double>(remainingCells(a.id)) - mean;
    variance += diff * diff;
  }
  variance /= static_cast<double>(agents_.size());
  return std::sqrt(variance);
}

void SwarmState::assignEqualStrips()
{
  const int n = agentCount();
  const int cols = area_.cols();

  for (auto & a : agents_) {
    a.region.clear();
    a.next_waypoint = 0;
  }

  // Sutunlari N serite mumkun oldugunca esit dagit; artan sutunlar bastan
  // itibaren birer birer paylastirilir.
  int col = 0;
  for (int i = 0; i < n; ++i) {
    const int base = cols / n;
    const int extra = (i < cols % n) ? 1 : 0;
    const int strip_cols = base + extra;
    std::vector<int> cells;
    for (int c = col; c < col + strip_cols; ++c) {
      for (int row = 0; row < area_.rows(); ++row) {
        cells.push_back(area_.cellId(c, row));
      }
    }
    col += strip_cols;
    // Bitisik ajanlara zit sutun yonu ver: suru duzeyinde serpantin desen.
    // Ayni yonde taransalardi tum ajanlar sabit sutun farkiyla kilitli ilerler
    // ve BIRBIRLERINE HIC YAKLASMAZDI -> hic karsilasma olayi dogmazdi.
    const bool reverse_columns = (i % 2) == 1;
    agents_[static_cast<std::size_t>(i)].sweep_reversed = reverse_columns;
    agents_[static_cast<std::size_t>(i)].region =
      area_.boustrophedonOrder(std::move(cells), reverse_columns);
  }

  // Her ajani kendi seridinin ilk waypoint'ine yerlestir.
  for (auto & a : agents_) {
    if (!a.region.empty()) {
      a.position = area_.cellCenter(a.region.front());
    }
  }
}

void SwarmState::randomizeLaunchPositions(int seed)
{
  std::mt19937 rng(static_cast<std::mt19937::result_type>(seed));
  std::uniform_real_distribution<double> x_axis(0.0, area_.cols() * area_.cellSize());
  std::uniform_real_distribution<double> y_axis(0.0, area_.rows() * area_.cellSize());
  for (auto & agent : agents_) {
    agent.position = Vec2{x_axis(rng), y_axis(rng)};
  }
}

SwarmState makeSwarmState(const ExperimentConfig & config, int seed)
{
  config.validate();
  SwarmState state(config.missionArea(), config.n_agents, config.sim.pheromone_decay);
  if (config.sim.random_launch) {
    // Kalkis konumlari ATAMADAN ONCE belirlenir: ihaleli algoritmalar
    // (P3b/P3c) tekliflerini gercek konumlara gore verebilsin.
    state.randomizeLaunchPositions(seed);
  }
  allocateRegions(&state, config.p3, config.sim.random_launch);
  return state;
}

SwarmState makeSwarmState(const ExperimentConfig & config)
{
  return makeSwarmState(config, 0);
}

}  // namespace swarm_bt_core
