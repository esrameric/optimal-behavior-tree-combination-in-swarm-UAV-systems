#include "swarm_bt_sim/episode_runner.hpp"

#include <memory>

namespace swarm_bt_sim
{

namespace
{

KinematicSimConfig toSimConfig(const swarm_bt_core::ExperimentConfig & config, int seed)
{
  KinematicSimConfig sim_config;
  sim_config.speed = config.sim.speed;
  sim_config.dt = config.sim.dt;
  sim_config.time_limit = config.sim.time_limit;
  sim_config.waypoint_tolerance = config.sim.waypoint_tolerance;
  sim_config.interest_deposit = config.sim.interest_deposit;
  sim_config.speed_jitter = config.sim.speed_jitter;
  sim_config.seed = seed;
  return sim_config;
}

}  // namespace

EpisodeRunner::EpisodeRunner(const swarm_bt_core::ExperimentConfig & config, int seed)
: config_(config),
  state_(swarm_bt_core::makeSwarmState(config, seed)),
  detector_(config.r_comm, config.encounter_hysteresis),
  negotiator_(config.swap_threshold),
  failure_injector_(config.failure, seed)
{
  sim_ = std::make_unique<KinematicSim>(state_, toSimConfig(config, seed));
}

void EpisodeRunner::triggerCoordination()
{
  switch (config_.p6) {
    case swarm_bt_core::TriggerModel::kEventDriven: {
        // P6c: yalnizca comm-range GIRISI karar tetikler. Aradaki tick'lerde
        // hicbir koordinasyon isi yapilmaz.
        ++metrics_.detection_checks;
        for (const auto & encounter : detector_.update(state_)) {
          handleEncounter(encounter.agent_a, encounter.agent_b);
        }
        break;
      }

    case swarm_bt_core::TriggerModel::kEveryTick: {
        // P6b: her tick, menzilde OLAN tum ciftler icin karar yeniden
        // degerlendirilir. En yuksek tepki hizi, en yuksek maliyet ve en cok
        // yeniden-atama firsati.
        ++metrics_.detection_checks;
        detector_.update(state_);
        for (const auto & pair : detector_.currentPairs()) {
          handleEncounter(pair.first, pair.second);
        }
        break;
      }

    case swarm_bt_core::TriggerModel::kPeriodicPolling: {
        // P6a: yoklama yalnizca poll_period'da bir yapilir. Iki yoklama
        // arasinda girip cikan ciftler HIC GORULMEZ -- ucuz ama kayipli.
        if (state_.time() + 1e-9 < next_poll_time_) {
          return;
        }
        next_poll_time_ = state_.time() + config_.sim.poll_period;
        ++metrics_.detection_checks;
        detector_.update(state_);
        for (const auto & pair : detector_.currentPairs()) {
          handleEncounter(pair.first, pair.second);
        }
        break;
      }
  }
}

void EpisodeRunner::handleEncounter(int agent_a, int agent_b)
{
  ++metrics_.coordination_events;
  bool changed = false;

  // Bolum 2.3: arizadan kalan sahipsiz alan once devralinir.
  const int transferred =
    swarm_bt_core::AreaSwapNegotiator::distributeOrphans(&state_, agent_a, agent_b);
  if (transferred > 0) {
    metrics_.orphan_transfers += transferred;
    changed = true;
  }

  // Bolum 2.2: dengesizlik esigi asilmissa takas degerlendirilir.
  const auto proposal = negotiator_.buildProposal(state_, agent_a, agent_b);
  if (proposal.has_value()) {
    ++metrics_.proposals;
    if (proposal->reducesTotalDistance()) {
      negotiator_.apply(&state_, *proposal);
      ++metrics_.swaps;
      changed = true;
    }
  }

  if (changed) {
    ++metrics_.churn_events;
  }
}

EpisodeMetrics EpisodeRunner::run()
{
  while (!sim_->finished()) {
    sim_->step();
    failure_injector_.update(&state_);

    triggerCoordination();

    // Kendi alanini bitiren ajan, sahipsiz kalmis alani ustlenir. Yalnizca
    // karsilasmaya dayali devralma yetmiyor (bkz. claimOrphansIfIdle).
    if (!state_.orphanedCells().empty()) {
      for (const auto & agent : state_.agents()) {
        const int claimed =
          swarm_bt_core::AreaSwapNegotiator::claimOrphansIfIdle(&state_, agent.id);
        if (claimed > 0) {
          metrics_.orphan_transfers += claimed;
          // churn_events'e SAYILMAZ: churn orani, karsilasmalarin ne kadarinin
          // gercek degisiklige yol actigini olcer. Bosta kalan ajanin devralmasi
          // bir karsilasma sonucu degildir; ayri olarak idle_claims'te tutulur.
          ++metrics_.idle_claims;
          break;
        }
      }
    }
  }

  double changes = 0.0;
  for (const auto & agent : state_.agents()) {
    changes += agent.assignment_changes;
    metrics_.total_distance += agent.distance_travelled;
  }

  // Karsilasma sikligi her zaman comm-range GIRIS sayisidir (Bolum 6 tanimi);
  // P6b'de koordinasyon karari cok daha sik degerlendirilir ama karsilasma
  // sayisi degismez.
  metrics_.encounters = detector_.totalEncounters();
  metrics_.mission_time = state_.time();
  metrics_.ticks = sim_->tickCount();
  metrics_.coverage_complete = state_.coverageComplete();
  metrics_.assignment_stability = changes / state_.agentCount();
  metrics_.coverage_imbalance = state_.coverageImbalance();
  metrics_.churn_ratio = (metrics_.encounters > 0) ?
    static_cast<double>(metrics_.churn_events) / metrics_.encounters :
    0.0;
  metrics_.failed_agent = failure_injector_.failedAgent();
  metrics_.failure_time = failure_injector_.failureTime();
  return metrics_;
}

}  // namespace swarm_bt_sim
