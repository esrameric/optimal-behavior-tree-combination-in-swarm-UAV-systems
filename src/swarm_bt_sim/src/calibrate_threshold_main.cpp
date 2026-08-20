// esik_degeri (takas tetikleme esigi) kalibrasyon taramasi - plan Bolum 2.2 / 9.
//
// Plan Bolum 9'daki uyari: "esik_degeri cok dusukse sistem surekli takas yapar
// (gurultu), cok yuksekse hic takas olmaz". Bu arac esigi tarayarak iki ucu da
// veriyle gosterir.
//
// Tarama IKI senaryoda kosulur:
//   arizasiz : esit seritli baseline. Ajanlar ayni oranda ilerledigi icin
//              dengesizligin buyuyecek bir kaynagi yoktur.
//   arizali  : Bolum 2.3 surpriz olayi acik. Ariza, yeniden-atama mekanizmasini
//              gercekten calistiran ana dengesizlik kaynagidir.
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include <swarm_bt_core/experiment_config.hpp>

#include "swarm_bt_sim/episode_runner.hpp"

namespace
{

double mean(const std::vector<double> & values)
{
  if (values.empty()) {
    return 0.0;
  }
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

struct Aggregate
{
  double encounters{0.0};
  double proposals{0.0};
  double swaps{0.0};
  double orphan_transfers{0.0};
  double churn_ratio{0.0};
  double assignment_stability{0.0};
  double mission_time{0.0};
  double coverage_imbalance{0.0};
  double completion_rate{0.0};
};

Aggregate sweepPoint(const swarm_bt_core::ExperimentConfig & config, int repetitions)
{
  std::vector<double> encounters;
  std::vector<double> proposals;
  std::vector<double> swaps;
  std::vector<double> orphans;
  std::vector<double> churn;
  std::vector<double> stability;
  std::vector<double> times;
  std::vector<double> imbalances;
  int completed = 0;

  for (int seed = 0; seed < repetitions; ++seed) {
    const auto metrics = swarm_bt_sim::EpisodeRunner(config, seed).run();
    encounters.push_back(metrics.encounters);
    proposals.push_back(metrics.proposals);
    swaps.push_back(metrics.swaps);
    orphans.push_back(metrics.orphan_transfers);
    churn.push_back(metrics.churn_ratio);
    stability.push_back(metrics.assignment_stability);
    times.push_back(metrics.mission_time);
    imbalances.push_back(metrics.coverage_imbalance);
    completed += metrics.coverage_complete ? 1 : 0;
  }

  Aggregate result;
  result.encounters = mean(encounters);
  result.proposals = mean(proposals);
  result.swaps = mean(swaps);
  result.orphan_transfers = mean(orphans);
  result.churn_ratio = mean(churn);
  result.assignment_stability = mean(stability);
  result.mission_time = mean(times);
  result.coverage_imbalance = mean(imbalances);
  result.completion_rate = static_cast<double>(completed) / repetitions;
  return result;
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

    std::cout << "senaryo,esik_degeri,N,karsilasma_ort,teklif_ort,takas_ort,"
              << "devralinan_hucre_ort,churn_orani,atama_kararliligi,"
              << "gorev_suresi_ort,kapsama_dengesizligi_ort,kapsama_tamam_orani,tekrar\n";
    std::cout << std::fixed << std::setprecision(4);

    for (const bool with_failure : {false, true}) {
      for (int percent = 0; percent <= 90; percent += 5) {
        auto config = base;
        config.swap_threshold = percent / 100.0;
        config.failure.enabled = with_failure;
        config.failure.time = -1.0;    // gorev ortasi
        config.failure.agent_id = -1;  // rastgele

        for (const int n : {3, 5}) {
          config.n_agents = n;
          const auto point = sweepPoint(config, repetitions);
          std::cout << (with_failure ? "arizali" : "arizasiz") << ","
                    << config.swap_threshold << "," << n << ","
                    << point.encounters << "," << point.proposals << ","
                    << point.swaps << "," << point.orphan_transfers << ","
                    << point.churn_ratio << "," << point.assignment_stability << ","
                    << point.mission_time << "," << point.coverage_imbalance << ","
                    << point.completion_rate << "," << repetitions << "\n";
        }
      }
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
