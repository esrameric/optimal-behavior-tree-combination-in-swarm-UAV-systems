#include "swarm_bt_core/region_allocator.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

#include "swarm_bt_core/area_swap.hpp"

namespace swarm_bt_core
{

namespace
{

/// Ajanlarin bolgelerini temizler ama kalkis konumlarini ve supurme yonlerini
/// korur: atama algoritmalari ayni baslangic geometrisinden calismali.
void clearRegionsKeepingLaunchState(SwarmState * state)
{
  for (auto & agent : state->agents()) {
    agent.region.clear();
    agent.next_waypoint = 0;
  }
}

/// Kapasite tavani: hicbir ajan bunun uzerinde sutun alamaz.
int columnCapacity(int column_count, int agent_count)
{
  return (column_count + agent_count - 1) / agent_count;
}

void assignColumn(SwarmState * state, int agent_id, int column)
{
  auto & agent = state->agent(agent_id);
  for (const int cell_id : columnCells(state->area(), column)) {
    agent.region.push_back(cell_id);
  }
  state->resequenceRegion(agent_id);
}

}  // namespace

std::vector<int> columnCells(const MissionArea & area, int column)
{
  if (column < 0 || column >= area.cols()) {
    throw std::out_of_range("columnCells: gecersiz sutun");
  }
  std::vector<int> cells;
  cells.reserve(static_cast<std::size_t>(area.rows()));
  for (int row = 0; row < area.rows(); ++row) {
    cells.push_back(area.cellId(column, row));
  }
  return cells;
}

double marginalColumnCost(const SwarmState & state, int agent_id, int column)
{
  const auto cells = columnCells(state.area(), column);
  return AreaSwapNegotiator::tourLengthWith(state, agent_id, cells) -
         AreaSwapNegotiator::tourLength(state, agent_id);
}

void allocateRegions(
  SwarmState * state, AllocationAlgorithm algorithm, bool keep_positions)
{
  if (state == nullptr) {
    throw std::invalid_argument("allocateRegions: durum bos olamaz");
  }

  // Serit atamasi hem P3a'nin sonucu hem de digerleri icin supurme yonlerinin
  // kaynagi; kalkis konumlari korunacaksa once saklanip sonra geri konur.
  std::vector<Vec2> launch_positions;
  if (keep_positions) {
    for (const auto & agent : state->agents()) {
      launch_positions.push_back(agent.position);
    }
  }

  state->assignEqualStrips();

  if (keep_positions) {
    for (auto & agent : state->agents()) {
      agent.position = launch_positions[static_cast<std::size_t>(agent.id)];
    }
  }

  if (algorithm == AllocationAlgorithm::kStaticEqual) {
    return;
  }

  const int column_count = state->area().cols();
  const int agent_count = state->agentCount();
  const int capacity = columnCapacity(column_count, agent_count);

  clearRegionsKeepingLaunchState(state);
  std::vector<int> assigned_columns(static_cast<std::size_t>(agent_count), 0);

  if (algorithm == AllocationAlgorithm::kContractNet) {
    // Sutunlar soldan saga sirayla ihaleye cikar; en dusuk teklif kazanir.
    for (int column = 0; column < column_count; ++column) {
      int winner = -1;
      double best_bid = std::numeric_limits<double>::infinity();
      for (int agent_id = 0; agent_id < agent_count; ++agent_id) {
        if (assigned_columns[static_cast<std::size_t>(agent_id)] >= capacity) {
          continue;
        }
        const double bid = marginalColumnCost(*state, agent_id, column);
        if (bid < best_bid) {
          best_bid = bid;
          winner = agent_id;
        }
      }
      if (winner < 0) {
        throw std::runtime_error("Contract Net: kapasitesi kalan ajan yok");
      }
      assignColumn(state, winner, column);
      ++assigned_columns[static_cast<std::size_t>(winner)];
    }
    return;
  }

  // CBBA: her adimda tum (ajan, atanmamis sutun) ciftleri arasindan marjinal
  // maliyeti en dusuk olani sec (en-iyi-once). Duyuru sirasindan bagimsizdir.
  std::vector<bool> taken(static_cast<std::size_t>(column_count), false);
  for (int remaining = column_count; remaining > 0; --remaining) {
    int best_agent = -1;
    int best_column = -1;
    double best_bid = std::numeric_limits<double>::infinity();

    for (int column = 0; column < column_count; ++column) {
      if (taken[static_cast<std::size_t>(column)]) {
        continue;
      }
      for (int agent_id = 0; agent_id < agent_count; ++agent_id) {
        if (assigned_columns[static_cast<std::size_t>(agent_id)] >= capacity) {
          continue;
        }
        const double bid = marginalColumnCost(*state, agent_id, column);
        if (bid < best_bid) {
          best_bid = bid;
          best_agent = agent_id;
          best_column = column;
        }
      }
    }

    if (best_agent < 0) {
      throw std::runtime_error("CBBA: kapasitesi kalan ajan yok");
    }
    assignColumn(state, best_agent, best_column);
    taken[static_cast<std::size_t>(best_column)] = true;
    ++assigned_columns[static_cast<std::size_t>(best_agent)];
  }
}

}  // namespace swarm_bt_core
