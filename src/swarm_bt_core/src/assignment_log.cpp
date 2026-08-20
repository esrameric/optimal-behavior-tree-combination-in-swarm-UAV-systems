#include "swarm_bt_core/assignment_log.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace swarm_bt_core
{

std::vector<AssignmentChangeEvent> AssignmentLog::drain()
{
  std::vector<AssignmentChangeEvent> pending(
    events_.begin() + static_cast<std::ptrdiff_t>(published_), events_.end());
  published_ = events_.size();
  return pending;
}

int AssignmentLog::countByReason(AssignmentReason reason) const
{
  return static_cast<int>(
    std::count_if(
      events_.begin(), events_.end(),
      [reason](const AssignmentChangeEvent & event) {return event.reason == reason;}));
}

double AssignmentLog::changesPerAgent(int agent_count) const
{
  if (agent_count <= 0) {
    throw std::invalid_argument("AssignmentLog::changesPerAgent: ajan sayisi pozitif olmali");
  }
  return static_cast<double>(events_.size()) / agent_count;
}

void AssignmentLog::clear()
{
  events_.clear();
  published_ = 0;
}

}  // namespace swarm_bt_core
