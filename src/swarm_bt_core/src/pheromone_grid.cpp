#include "swarm_bt_core/pheromone_grid.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace swarm_bt_core
{

PheromoneGrid::PheromoneGrid(int cell_count, double decay_rate)
: values_(static_cast<std::size_t>(cell_count), 0.0), decay_rate_(decay_rate)
{
  if (cell_count <= 0) {
    throw std::invalid_argument("PheromoneGrid: hucre sayisi pozitif olmali");
  }
  if (decay_rate < 0.0 || decay_rate > 1.0) {
    throw std::invalid_argument("PheromoneGrid: decay_rate [0,1] araliginda olmali");
  }
}

double PheromoneGrid::at(int cell_id) const
{
  if (cell_id < 0 || cell_id >= cellCount()) {
    throw std::out_of_range("PheromoneGrid::at: gecersiz hucre kimligi");
  }
  return values_[static_cast<std::size_t>(cell_id)];
}

void PheromoneGrid::deposit(int cell_id, double amount)
{
  if (cell_id < 0 || cell_id >= cellCount()) {
    throw std::out_of_range("PheromoneGrid::deposit: gecersiz hucre kimligi");
  }
  values_[static_cast<std::size_t>(cell_id)] += amount;
}

void PheromoneGrid::set(int cell_id, double value)
{
  if (cell_id < 0 || cell_id >= cellCount()) {
    throw std::out_of_range("PheromoneGrid::set: gecersiz hucre kimligi");
  }
  values_[static_cast<std::size_t>(cell_id)] = value;
}

void PheromoneGrid::decay()
{
  if (decay_rate_ <= 0.0) {
    return;
  }
  const double factor = 1.0 - decay_rate_;
  for (auto & value : values_) {
    value *= factor;
    if (value < kEpsilon) {
      value = 0.0;
    }
  }
}

double PheromoneGrid::total() const
{
  return std::accumulate(values_.begin(), values_.end(), 0.0);
}

double PheromoneGrid::maxValue() const
{
  if (values_.empty()) {
    return 0.0;
  }
  return *std::max_element(values_.begin(), values_.end());
}

double PheromoneGrid::meanOver(const std::vector<int> & cell_ids) const
{
  if (cell_ids.empty()) {
    return 0.0;
  }
  double sum = 0.0;
  for (const int cell_id : cell_ids) {
    sum += at(cell_id);
  }
  return sum / static_cast<double>(cell_ids.size());
}

void PheromoneGrid::reset()
{
  std::fill(values_.begin(), values_.end(), 0.0);
}

}  // namespace swarm_bt_core
