#include "swarm_bt_core/failure_injector.hpp"

#include <optional>
#include <random>
#include <stdexcept>
#include <vector>

namespace swarm_bt_core
{

double globalCoverageRatio(const SwarmState & state)
{
  const int total = state.area().cellCount();
  if (total == 0) {
    return 1.0;
  }
  int visited = 0;
  for (int cell_id = 0; cell_id < total; ++cell_id) {
    if (state.isVisited(cell_id)) {
      ++visited;
    }
  }
  return static_cast<double>(visited) / static_cast<double>(total);
}

FailureInjector::FailureInjector(const FailureInjection & config, int seed)
: config_(config), rng_(static_cast<std::mt19937::result_type>(seed))
{
}

bool FailureInjector::shouldTrigger(const SwarmState & state) const
{
  if (config_.time >= 0.0) {
    return state.time() >= config_.time;
  }
  return globalCoverageRatio(state) >= kMidMissionCoverage;
}

int FailureInjector::selectAgent(const SwarmState & state)
{
  if (config_.agent_id >= 0) {
    return config_.agent_id;
  }

  std::vector<int> alive;
  for (const auto & agent : state.agents()) {
    if (agent.alive) {
      alive.push_back(agent.id);
    }
  }
  if (alive.empty()) {
    return -1;
  }
  std::uniform_int_distribution<std::size_t> pick(0, alive.size() - 1);
  return alive[pick(rng_)];
}

std::optional<int> FailureInjector::update(SwarmState * state)
{
  if (state == nullptr) {
    throw std::invalid_argument("FailureInjector::update: durum bos olamaz");
  }
  if (!config_.enabled || triggered_ || !shouldTrigger(*state)) {
    return std::nullopt;
  }

  const int agent_id = selectAgent(*state);
  if (agent_id < 0 || !state->agent(agent_id).alive) {
    return std::nullopt;
  }

  state->failAgent(agent_id);
  triggered_ = true;
  failed_agent_ = agent_id;
  failure_time_ = state->time();
  return agent_id;
}

}  // namespace swarm_bt_core
