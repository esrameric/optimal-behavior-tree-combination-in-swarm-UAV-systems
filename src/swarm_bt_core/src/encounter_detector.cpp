#include "swarm_bt_core/encounter_detector.hpp"

#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace swarm_bt_core
{

EncounterDetector::EncounterDetector(double r_comm)
: r_comm_(r_comm)
{
  if (r_comm <= 0.0) {
    throw std::invalid_argument("EncounterDetector: r_comm pozitif olmali");
  }
}

std::pair<int, int> EncounterDetector::orderedPair(int a, int b)
{
  return (a < b) ? std::make_pair(a, b) : std::make_pair(b, a);
}

std::vector<EncounterEvent> EncounterDetector::update(const SwarmState & state)
{
  std::vector<EncounterEvent> events;
  std::set<std::pair<int, int>> current;

  const auto & agents = state.agents();
  for (std::size_t i = 0; i < agents.size(); ++i) {
    if (!agents[i].alive) {
      continue;
    }
    for (std::size_t j = i + 1; j < agents.size(); ++j) {
      if (!agents[j].alive) {
        continue;
      }
      const double d = distance(agents[i].position, agents[j].position);
      if (d > r_comm_) {
        continue;
      }
      const auto key = orderedPair(agents[i].id, agents[j].id);
      current.insert(key);
      if (in_range_.count(key) == 0) {
        events.push_back(EncounterEvent{key.first, key.second, state.time(), d});
        ++total_encounters_;
      }
    }
  }

  in_range_ = std::move(current);
  return events;
}

void EncounterDetector::reset()
{
  in_range_.clear();
  total_encounters_ = 0;
}

}  // namespace swarm_bt_core
