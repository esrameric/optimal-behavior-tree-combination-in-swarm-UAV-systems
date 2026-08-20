#include "swarm_bt_sim/kinematic_sim.hpp"

#include <random>
#include <stdexcept>

namespace swarm_bt_sim
{

using swarm_bt_core::AgentState;
using swarm_bt_core::Vec2;

KinematicSim::KinematicSim(swarm_bt_core::SwarmState & state, const KinematicSimConfig & config)
: state_(&state), config_(config)
{
  if (config.speed <= 0.0 || config.dt <= 0.0) {
    throw std::invalid_argument("KinematicSim: hiz ve dt pozitif olmali");
  }
  if (config.speed_jitter < 0.0 || config.speed_jitter >= 1.0) {
    throw std::invalid_argument("KinematicSim: speed_jitter [0,1) araliginda olmali");
  }

  // Ajan basina hiz carpani, tohumdan tekrarlanabilir sekilde cekilir.
  std::mt19937 rng(static_cast<std::mt19937::result_type>(config.seed));
  std::uniform_real_distribution<double> jitter(
    1.0 - config.speed_jitter, 1.0 + config.speed_jitter);
  speed_factors_.resize(static_cast<std::size_t>(state.agentCount()));
  for (auto & factor : speed_factors_) {
    factor = (config.speed_jitter > 0.0) ? jitter(rng) : 1.0;
  }
}

double KinematicSim::speedOf(int agent_id) const
{
  return config_.speed * speed_factors_[static_cast<std::size_t>(agent_id)];
}

int KinematicSim::nextTargetCell(AgentState & agent) const
{
  while (agent.next_waypoint < agent.region.size()) {
    const int cell_id = agent.region[agent.next_waypoint];
    if (!state_->knowsVisited(agent.id, cell_id)) {
      return cell_id;
    }
    // Ajan bu hucrenin tarandigini BILIYOR: atla. Stigmerji (P5b) kapaliysa
    // baskasinin taradigi hucreyi bilemez ve tekrar tarar.
    ++agent.next_waypoint;
  }
  return -1;
}

void KinematicSim::moveAgent(AgentState & agent)
{
  const int target_cell = nextTargetCell(agent);
  if (target_cell < 0) {
    return;
  }

  const Vec2 target = state_->area().cellCenter(target_cell);
  const Vec2 delta = target - agent.position;
  const double remaining = swarm_bt_core::norm(delta);
  const double step_length = speedOf(agent.id) * config_.dt;

  if (remaining <= step_length || remaining <= config_.waypoint_tolerance) {
    // Hedefe varildi: hucreyi tara, izini birak, siradakine gec.
    agent.distance_travelled += remaining;
    agent.position = target;
    state_->markVisitedBy(agent.id, target_cell);
    state_->interest().deposit(target_cell, config_.interest_deposit);
    ++agent.next_waypoint;
    return;
  }

  agent.position = agent.position + swarm_bt_core::normalized(delta) * step_length;
  agent.distance_travelled += step_length;
}

void KinematicSim::step()
{
  for (auto & agent : state_->agents()) {
    if (!agent.alive) {
      continue;
    }
    moveAgent(agent);
  }

  state_->interest().decay();
  ++tick_count_;
  // Zamani tick sayisindan turet: dt birikimi kayan nokta kaymasi yaratiyor.
  state_->setTime(tick_count_ * config_.dt);
}

bool KinematicSim::finished() const
{
  return state_->coverageComplete() || state_->time() >= config_.time_limit;
}

}  // namespace swarm_bt_sim
