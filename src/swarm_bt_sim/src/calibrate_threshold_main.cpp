// esik_degeri (takas tetikleme esigi) kalibrasyon taramasi - plan Bolum 2.2 / 9.
//
// Plan Bolum 9'daki uyari: "esik_degeri cok dusukse sistem surekli takas yapar
// (gurultu), cok yuksekse hic takas olmaz". Bu arac esigi tarayarak iki ucu da
// veriyle gosterir ve aradaki calisma bandini bulur.
//
// Negotiation BT alt-agaci Bolum 5/Faz 0'da yazilacak; takas mekanizmasi
// BT'den bagimsiz oldugu icin kalibrasyon dogrudan AreaSwapNegotiator uzerinden
// yapilir. Ayni mekanizma daha sonra BT dugumlerinin arkasina baglanacak.
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include <swarm_bt_core/area_swap.hpp>
#include <swarm_bt_core/encounter_detector.hpp>
#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_core/swarm_state.hpp>

#include "swarm_bt_sim/kinematic_sim.hpp"

namespace
{

struct EpisodeResult
{
  int encounters{0};
  /// Dengesizlik esigini asip teklif kurulan karsilasma sayisi.
  int proposals{0};
  int swaps{0};
  /// Kurulan tekliflerin ortalama fayda farki (proposer_gain - receiver_cost).
  double mean_benefit{0.0};
  double mission_time{0.0};
  double coverage_imbalance{0.0};
  double assignment_changes_per_agent{0.0};
  bool coverage_complete{false};
};

EpisodeResult runEpisode(const swarm_bt_core::ExperimentConfig & config, int seed)
{
  auto state = swarm_bt_core::makeSwarmState(config);

  swarm_bt_sim::KinematicSimConfig sim_config;
  sim_config.speed = config.sim.speed;
  sim_config.dt = config.sim.dt;
  sim_config.time_limit = config.sim.time_limit;
  sim_config.waypoint_tolerance = config.sim.waypoint_tolerance;
  sim_config.interest_deposit = config.sim.interest_deposit;
  sim_config.speed_jitter = config.sim.speed_jitter;
  sim_config.seed = seed;

  swarm_bt_core::EncounterDetector detector(config.r_comm, config.encounter_hysteresis);
  const swarm_bt_core::AreaSwapNegotiator negotiator(config.swap_threshold);
  swarm_bt_sim::KinematicSim sim(state, sim_config);

  EpisodeResult result;
  double benefit_sum = 0.0;
  while (!sim.finished()) {
    sim.step();
    for (const auto & encounter : detector.update(state)) {
      ++result.encounters;
      const auto proposal =
        negotiator.buildProposal(state, encounter.agent_a, encounter.agent_b);
      if (!proposal.has_value()) {
        continue;
      }
      ++result.proposals;
      benefit_sum += proposal->proposer_gain - proposal->receiver_cost;
      if (proposal->reducesTotalDistance()) {
        negotiator.apply(&state, *proposal);
        ++result.swaps;
      }
    }
  }

  double changes = 0.0;
  for (const auto & agent : state.agents()) {
    changes += agent.assignment_changes;
  }

  result.mean_benefit = (result.proposals > 0) ? benefit_sum / result.proposals : 0.0;
  result.mission_time = state.time();
  result.coverage_imbalance = state.coverageImbalance();
  result.assignment_changes_per_agent = changes / state.agentCount();
  result.coverage_complete = state.coverageComplete();
  return result;
}

double mean(const std::vector<double> & values)
{
  if (values.empty()) {
    return 0.0;
  }
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

}  // namespace

int main(int argc, char ** argv)
{
  int repetitions = 10;
  swarm_bt_core::ExperimentConfig base;

  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--repetitions", [&](const std::string & v) {repetitions = std::stoi(v);}},
    {"--r-comm", [&](const std::string & v) {base.r_comm = std::stod(v);}},
    {"--area", [&](const std::string & v) {base.sim.area_side = std::stod(v);}},
  };

  try {
    for (int i = 1; i < argc; ++i) {
      const std::string flag = argv[i];
      if (flag == "--help") {
        std::cout << "Kullanim: " << argv[0]
                  << " [--repetitions <n>] [--r-comm <m>] [--area <m>]\nCSV stdout'a yazilir.\n";
        return 0;
      }
      const auto handler = handlers.find(flag);
      if (handler == handlers.end() || i + 1 >= argc) {
        std::cerr << "Gecersiz secenek: " << flag << "\n";
        return 1;
      }
      handler->second(argv[++i]);
    }

    std::cout << "esik_degeri,N,karsilasma_ort,teklif_ort,takas_ort,churn_orani,"
              << "ortalama_fayda,atama_degisikligi_ajan_basina,gorev_suresi_ort,"
              << "kapsama_dengesizligi_ort,tekrar\n";
    std::cout << std::fixed << std::setprecision(4);

    for (int percent = 0; percent <= 90; percent += 5) {
      auto config = base;
      config.swap_threshold = percent / 100.0;
      for (const int n : {3, 5}) {
        config.n_agents = n;
        std::vector<double> encounters;
        std::vector<double> proposals;
        std::vector<double> benefits;
        std::vector<double> swaps;
        std::vector<double> times;
        std::vector<double> imbalances;
        std::vector<double> changes;
        for (int seed = 0; seed < repetitions; ++seed) {
          const auto result = runEpisode(config, seed);
          encounters.push_back(result.encounters);
          proposals.push_back(result.proposals);
          benefits.push_back(result.mean_benefit);
          swaps.push_back(result.swaps);
          times.push_back(result.mission_time);
          imbalances.push_back(result.coverage_imbalance);
          changes.push_back(result.assignment_changes_per_agent);
        }
        const double encounter_mean = mean(encounters);
        const double churn = (encounter_mean > 0.0) ? mean(swaps) / encounter_mean : 0.0;
        std::cout << config.swap_threshold << "," << n << ","
                  << encounter_mean << "," << mean(proposals) << "," << mean(swaps) << ","
                  << churn << "," << mean(benefits) << ","
                  << mean(changes) << "," << mean(times) << ","
                  << mean(imbalances) << "," << repetitions << "\n";
      }
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
