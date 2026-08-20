#include "swarm_bt_core/swarm_state.hpp"

#include <cmath>
#include <stdexcept>
#include <vector>

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

bool SwarmState::coverageComplete() const
{
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

SwarmState makeSwarmState(const ExperimentConfig & config)
{
  config.validate();
  SwarmState state(config.missionArea(), config.n_agents, config.sim.pheromone_decay);
  state.assignEqualStrips();
  return state;
}

}  // namespace swarm_bt_core
