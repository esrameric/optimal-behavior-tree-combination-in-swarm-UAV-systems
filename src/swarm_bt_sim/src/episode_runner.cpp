#include "swarm_bt_sim/episode_runner.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

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
  proximity_monitor_(config.sim.safety_radius, config.encounter_hysteresis),
  negotiator_(config.swap_threshold),
  failure_injector_(config.failure, seed)
{
  // P5b - stigmerji: acikken ajanlar cevredeki izleri okur, kapaliyken yalnizca
  // kendi taradiklarini bilir ve baskasinin taradigi hucreyi tekrar tarar.
  state_.setStigmergyEnabled(config.p5.stigmergy);
  sim_ = std::make_unique<KinematicSim>(state_, toSimConfig(config, seed));
}

void EpisodeRunner::applyEavesdropping(int agent_a, int agent_b)
{
  // P5d: muzakere eden ciftin menzilindeki ucuncu ajanlar konusmayi duyar ve
  // stigmerji bilgisini gunceller. Dogrudan mesaj gerektirmez.
  for (const auto & listener : state_.agents()) {
    if (!listener.alive || listener.id == agent_a || listener.id == agent_b) {
      continue;
    }
    const double to_a =
      swarm_bt_core::distance(listener.position, state_.agent(agent_a).position);
    const double to_b =
      swarm_bt_core::distance(listener.position, state_.agent(agent_b).position);
    if (std::min(to_a, to_b) > config_.r_comm) {
      continue;
    }
    metrics_.shared_cell_updates += state_.shareKnowledge(listener.id, agent_a);
    metrics_.shared_cell_updates += state_.shareKnowledge(listener.id, agent_b);
    ++metrics_.eavesdrop_events;
  }
}

void EpisodeRunner::triggerCoordination()
{
  switch (config_.p6) {
    case swarm_bt_core::TriggerModel::kEventDriven: {
        // P6c: yalnizca comm-range GIRISI karar tetikler. Aradaki tick'lerde
        // hicbir koordinasyon isi yapilmaz.
        ++metrics_.detection_checks;
        std::vector<std::pair<int, int>> entries;
        for (const auto & encounter : detector_.update(state_)) {
          entries.emplace_back(encounter.agent_a, encounter.agent_b);
        }
        // Merkezi mimaride ariza da bir olaydir: merkez, karsilasma beklemeden
        // sahipsiz alani dagitabilmeli.
        const bool central =
          config_.p2 == swarm_bt_core::CoordinationArchitecture::kCentral;
        if (entries.empty() && !(central && !state_.orphanedCells().empty())) {
          return;
        }
        runCoordination(entries);
        break;
      }

    case swarm_bt_core::TriggerModel::kEveryTick: {
        // P6b: her tick, menzilde OLAN tum ciftler icin karar yeniden
        // degerlendirilir. En yuksek tepki hizi, en yuksek maliyet ve en cok
        // yeniden-atama firsati.
        ++metrics_.detection_checks;
        detector_.update(state_);
        runCoordination(
          std::vector<std::pair<int, int>>(
            detector_.currentPairs().begin(), detector_.currentPairs().end()));
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
        runCoordination(
          std::vector<std::pair<int, int>>(
            detector_.currentPairs().begin(), detector_.currentPairs().end()));
        break;
      }
  }
}

bool EpisodeRunner::rebalancePair(int agent_a, int agent_b)
{
  bool changed = false;

  // P5a - dogrudan mesaj: handshake sirasinda durum bilgisi degisilir ve
  // stigmerji senkronize edilir (plan Bolum 2.2:
  // sadece_bilgi_paylas_ve_devam_et).
  if (config_.p5.direct_message) {
    metrics_.shared_cell_updates += state_.shareKnowledge(agent_a, agent_b);
  }

  // P5d - kulak misafiri: menzildeki ucuncu taraflar da duyar.
  if (config_.p5.eavesdrop) {
    applyEavesdropping(agent_a, agent_b);
  }

  // Bolum 2.3: arizadan kalan sahipsiz alan once devralinir.
  const int transferred =
    swarm_bt_core::AreaSwapNegotiator::distributeOrphans(&state_, agent_a, agent_b);
  if (transferred > 0) {
    metrics_.orphan_transfers += transferred;
    changed = true;
  }

  // Bolum 2.2: takas muzakeresi DOGRUDAN MESAJ gerektirir. P5a kapaliysa
  // ajanlar birbirinin kalan alanini ogrenemez, dolayisiyla dengesizlik
  // hesaplanamaz ve takas teklifi kurulamaz.
  if (config_.p5.direct_message) {
    const auto proposal = negotiator_.buildProposal(state_, agent_a, agent_b);
    if (proposal.has_value()) {
      ++metrics_.proposals;
      if (proposal->reducesTotalDistance()) {
        negotiator_.apply(&state_, *proposal);
        ++metrics_.swaps;
        changed = true;
      }
    }
  }

  return changed;
}

std::pair<int, int> EpisodeRunner::mostImbalancedPair(const std::vector<int> & agent_ids) const
{
  int busiest = -1;
  int idlest = -1;
  double highest = -1.0;
  double lowest = 2.0;
  for (const int id : agent_ids) {
    if (!state_.agent(id).alive) {
      continue;
    }
    const double ratio = state_.remainingRatio(id);
    if (ratio > highest) {
      highest = ratio;
      busiest = id;
    }
    if (ratio < lowest) {
      lowest = ratio;
      idlest = id;
    }
  }
  return {busiest, idlest};
}

void EpisodeRunner::handleEncounter(int agent_a, int agent_b)
{
  ++metrics_.coordination_events;
  metrics_.coordination_messages += 2;   // ikili handshake
  if (rebalancePair(agent_a, agent_b)) {
    ++metrics_.churn_events;
  }
}

void EpisodeRunner::runCentralCoordination()
{
  // P2a: merkez tum ajanlarin durumunu gorur; comm-range gerekmez. Her
  // koordinasyon adiminda tum ajanlar merkeze rapor verir.
  ++metrics_.coordination_events;
  metrics_.coordination_messages += state_.agentCount();

  std::vector<int> alive;
  for (const auto & agent : state_.agents()) {
    if (agent.alive) {
      alive.push_back(agent.id);
    }
  }
  if (alive.size() < 2) {
    return;
  }

  bool changed = false;

  // Sahipsiz alan kuresel olarak dagitilir: merkez, her hucreyi o an en yakin
  // canli ajana verir. Karsilasma beklemek gerekmez -- merkezi mimarinin
  // dagitik mimariden en belirgin farki budur (bkz. README V12).
  if (!state_.orphanedCells().empty()) {
    for (std::size_t i = 0; i + 1 < alive.size(); ++i) {
      const int transferred = swarm_bt_core::AreaSwapNegotiator::distributeOrphans(
        &state_, alive[i], alive[i + 1]);
      if (transferred > 0) {
        metrics_.orphan_transfers += transferred;
        changed = true;
      }
    }
  }

  // Kuresel dengeleme: en yuklu ve en bos ajan arasinda takas degerlendirilir.
  const auto pair = mostImbalancedPair(alive);
  if (pair.first >= 0 && pair.second >= 0 && pair.first != pair.second) {
    const auto proposal = negotiator_.buildProposal(state_, pair.first, pair.second);
    if (proposal.has_value()) {
      ++metrics_.proposals;
      if (proposal->reducesTotalDistance()) {
        negotiator_.apply(&state_, *proposal);
        ++metrics_.swaps;
        changed = true;
      }
    }
  }

  if (changed) {
    ++metrics_.churn_events;
  }
}

void EpisodeRunner::runHierarchicalCoordination(const std::vector<std::pair<int, int>> & pairs)
{
  if (pairs.empty()) {
    return;
  }

  // Menzil grafiginin bagli bilesenleri = kumeler (birlestir-bul).
  std::vector<int> parent(static_cast<std::size_t>(state_.agentCount()));
  std::iota(parent.begin(), parent.end(), 0);
  std::function<int(int)> find = [&](int x) {
      while (parent[static_cast<std::size_t>(x)] != x) {
        parent[static_cast<std::size_t>(x)] =
          parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(x)])];
        x = parent[static_cast<std::size_t>(x)];
      }
      return x;
    };
  for (const auto & pair : pairs) {
    parent[static_cast<std::size_t>(find(pair.first))] = find(pair.second);
  }

  std::map<int, std::vector<int>> clusters;
  for (const auto & agent : state_.agents()) {
    if (agent.alive) {
      clusters[find(agent.id)].push_back(agent.id);
    }
  }

  for (const auto & entry : clusters) {
    const auto & members = entry.second;
    if (members.size() < 2) {
      continue;
    }

    // Gecici lider secilir; kume uyeleri durumlarini lidere bildirir.
    ++metrics_.leader_elections;
    if (members.size() >= 3) {
      ++metrics_.multi_agent_clusters;
    }
    ++metrics_.coordination_events;
    metrics_.coordination_messages += static_cast<int>(members.size());

    // Lider KUMENIN TAMAMI icin plan yapar: ikili pazarlikta her cift yalnizca
    // kendi arasinda dengelenirken, lider kumeyi butun olarak gorup ardisik
    // dengeleme adimlari uygular. Dagitik mimariden farki budur; kume iki
    // kisilikse ikisi ayni sonuca varir.
    bool cluster_changed = false;
    if (!state_.orphanedCells().empty()) {
      for (std::size_t i = 0; i + 1 < members.size(); ++i) {
        const int transferred = swarm_bt_core::AreaSwapNegotiator::distributeOrphans(
          &state_, members[i], members[i + 1]);
        if (transferred > 0) {
          metrics_.orphan_transfers += transferred;
          cluster_changed = true;
        }
      }
    }

    for (std::size_t round = 0; round + 1 < members.size(); ++round) {
      const auto pair = mostImbalancedPair(members);
      if (pair.first < 0 || pair.second < 0 || pair.first == pair.second) {
        break;
      }
      if (!rebalancePair(pair.first, pair.second)) {
        break;   // dengeleme fayda vermiyor, tur bitti
      }
      cluster_changed = true;
    }

    if (cluster_changed) {
      ++metrics_.churn_events;
    }
  }
}

void EpisodeRunner::runCoordination(const std::vector<std::pair<int, int>> & pairs)
{
  switch (config_.p2) {
    case swarm_bt_core::CoordinationArchitecture::kCentral:
      runCentralCoordination();
      break;
    case swarm_bt_core::CoordinationArchitecture::kHierarchicalHybrid:
      runHierarchicalCoordination(pairs);
      break;
    case swarm_bt_core::CoordinationArchitecture::kDistributed:
      for (const auto & pair : pairs) {
        handleEncounter(pair.first, pair.second);
      }
      break;
  }
}

EpisodeMetrics EpisodeRunner::run()
{
  while (!sim_->finished()) {
    sim_->step();
    failure_injector_.update(&state_);

    // Guvenlik yaricapi ihlalleri, koordinasyon modelinden bagimsiz olarak
    // her tick olculur (P6 yoklama araligindan etkilenmemeli).
    proximity_monitor_.update(state_);

    triggerCoordination();

    // Kendi alanini bitiren ajan, sahipsiz kalmis alani ustlenir. Yalnizca
    // karsilasmaya dayali devralma yetmiyor (bkz. claimOrphansIfIdle).
    //
    // P5c - intent yayini: sahipsiz alanin varligi suruye duyuruldugu icin
    // bosta kalan ajan bunu bir karsilasma beklemeden ogrenir. P5c kapaliysa
    // devralma yalnizca karsilasma aninda mumkundur.
    if (config_.p5.intent_broadcast && !state_.orphanedCells().empty()) {
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
  metrics_.collisions = proximity_monitor_.totalEncounters();

  // Ajanlarin bilgi kapsamasi: gercekte taranmis hucrelerin ne kadarini
  // biliyorlar. P5b (stigmerji) acikken 1.0; kapaliyken paylasilan bilgi kadar.
  int visited_total = 0;
  for (int cell_id = 0; cell_id < state_.area().cellCount(); ++cell_id) {
    if (state_.isVisited(cell_id)) {
      ++visited_total;
    }
  }
  if (visited_total > 0) {
    double known_sum = 0.0;
    for (const auto & agent : state_.agents()) {
      int known = 0;
      for (int cell_id = 0; cell_id < state_.area().cellCount(); ++cell_id) {
        if (state_.isVisited(cell_id) && state_.knowsVisited(agent.id, cell_id)) {
          ++known;
        }
      }
      known_sum += static_cast<double>(known) / visited_total;
    }
    metrics_.known_coverage_ratio = known_sum / state_.agentCount();
  }
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
