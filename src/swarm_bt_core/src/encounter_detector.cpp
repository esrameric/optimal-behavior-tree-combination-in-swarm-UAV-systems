#include "swarm_bt_core/encounter_detector.hpp"

#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace swarm_bt_core
{

EncounterDetector::EncounterDetector(double r_comm, double hysteresis_ratio)
: r_comm_(r_comm), hysteresis_ratio_(hysteresis_ratio)
{
  if (r_comm <= 0.0) {
    throw std::invalid_argument("EncounterDetector: r_comm pozitif olmali");
  }
  if (hysteresis_ratio < 0.0) {
    throw std::invalid_argument("EncounterDetector: hysteresis_ratio negatif olamaz");
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
      const auto key = orderedPair(agents[i].id, agents[j].id);
      const bool was_in_range = in_range_.count(key) > 0;

      if (was_in_range) {
        // Histerezis: cikis esigi asilmadikca cift menzilde sayilir.
        if (d <= exitRange()) {
          current.insert(key);
        }
        continue;
      }

      if (d <= r_comm_) {
        current.insert(key);
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
