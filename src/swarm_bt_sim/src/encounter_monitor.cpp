#include "swarm_bt_sim/encounter_monitor.hpp"

#include <stdexcept>
#include <vector>

namespace swarm_bt_sim
{

namespace
{

/// Izleyici yalnizca POZISYON tutar; bolge atamasina ihtiyaci yok. Bu yuzden
/// asgari bir alan ve sonumlemesiz feromon ile kurulur.
swarm_bt_core::SwarmState makePositionOnlyState(
  int agent_count, const swarm_bt_core::ExperimentConfig & config)
{
  return swarm_bt_core::SwarmState(config.missionArea(), agent_count, 0.0);
}

}  // namespace

EncounterMonitor::EncounterMonitor(
  int agent_count, const swarm_bt_core::ExperimentConfig & config)
: config_(config),
  state_(makePositionOnlyState(agent_count, config)),
  detector_(config.r_comm, config.encounter_hysteresis)
{
  if (agent_count <= 0) {
    throw std::invalid_argument("EncounterMonitor: ajan sayisi pozitif olmali");
  }
}

void EncounterMonitor::setPosition(int agent_id, double x, double y)
{
  state_.agent(agent_id).position = swarm_bt_core::Vec2{x, y};
}

void EncounterMonitor::setAlive(int agent_id, bool alive)
{
  state_.agent(agent_id).alive = alive;
}

std::vector<swarm_bt_core::EncounterEvent> EncounterMonitor::update(double time)
{
  state_.setTime(time);

  // P6a: yoklama yalnizca poll_period'da bir. Aradaki cagrilar hicbir sey
  // yapmaz -- ucuz ama iki yoklama arasinda girip cikan ciftleri kacirir.
  if (config_.p6 == swarm_bt_core::TriggerModel::kPeriodicPolling) {
    if (time + 1e-9 < next_poll_time_) {
      return {};
    }
    next_poll_time_ = time + config_.sim.poll_period;
  }

  ++check_count_;
  return detector_.update(state_);
}

}  // namespace swarm_bt_sim
