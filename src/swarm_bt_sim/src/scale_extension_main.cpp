// Bolum 9 istege bagli genisletmeleri: orantili alan ve ucuncu/dorduncu olcek.
//
// Iki kontrol kolu koşar:
//
//   sabit_alan   : gorev alani SABIT (bu calismanin ana kurulumu). N arttikca
//                  yogunluk artar; olculen fark hem N'in dogrudan etkisini hem
//                  de yogunlugun dolayli etkisini icerir.
//   orantili_alan: alan N ile ORANTILI buyur (drone basina duşen alan sabit).
//                  Karsilasma sikligi etkisi ortadan kalkar; kalan fark SAF
//                  olceklenebilirliktir.
//
// Her iki kol da N in {3, 5, 7, 10} icin koşulur -- plan Bolum 9'un "ucuncu bir
// N degeri eklemeyi dusunebilirsin" onerisi.
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_core/parameter_space.hpp>

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

void emitRow(
  const std::string & arm, const swarm_bt_core::ExperimentConfig & config, int repetitions)
{
  std::vector<double> times;
  std::vector<double> encounters;
  std::vector<double> stability;
  std::vector<double> churn;
  std::vector<double> communication;
  std::vector<double> swaps;
  int completed = 0;

  for (int seed = 0; seed < repetitions; ++seed) {
    const auto metrics = swarm_bt_sim::EpisodeRunner(config, seed).run();
    times.push_back(metrics.mission_time);
    encounters.push_back(metrics.encounters);
    stability.push_back(metrics.assignment_stability);
    churn.push_back(metrics.churn_ratio);
    communication.push_back(metrics.coordination_messages + metrics.shared_cell_updates);
    swaps.push_back(metrics.swaps);
    completed += metrics.coverage_complete ? 1 : 0;
  }

  const double cells_per_agent = config.cellsPerAgent();
  std::cout << arm << "," << config.n_agents << ","
            << config.sim.area_side << "," << config.r_comm << ","
            << cells_per_agent << ","
            << mean(encounters) << "," << mean(times) << ","
            << mean(stability) << "," << mean(churn) << ","
            << mean(communication) << "," << mean(swaps) << ","
            << static_cast<double>(completed) / repetitions << "," << repetitions << "\n";
  std::cerr << arm << " N=" << config.n_agents
            << " alan=" << config.sim.area_side
            << " karsilasma=" << mean(encounters) << "\n";
}

}  // namespace

int main(int argc, char ** argv)
{
  int repetitions = 10;

  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--repetitions", [&](const std::string & v) {repetitions = std::stoi(v);}},
  };

  try {
    for (int i = 1; i < argc; ++i) {
      const std::string flag = argv[i];
      if (flag == "--help") {
        std::cout << "Kullanim: " << argv[0] << " [--repetitions <n>]\nCSV stdout'a yazilir.\n";
        return 0;
      }
      const auto handler = handlers.find(flag);
      if (handler == handlers.end() || i + 1 >= argc) {
        std::cerr << "Gecersiz secenek: " << flag << "\n";
        return 1;
      }
      handler->second(argv[++i]);
    }

    auto base = swarm_bt_core::baselineConfig();
    base.failure.enabled = true;
    base.failure.time = -1.0;
    base.failure.agent_id = -1;

    std::cout << "kol,N,alan_kenari,r_comm,hucre_basina_ajan,karsilasma,"
              << "gorev_suresi,atama_kararliligi,churn_orani,iletisim_yuku,"
              << "takas,kapsama_tamam_orani,tekrar\n";
    std::cout << std::fixed << std::setprecision(4);

    const int reference = swarm_bt_core::scaleValues().front();

    for (const int n : swarm_bt_core::extendedScaleValues()) {
      auto fixed_area = base;
      fixed_area.n_agents = n;
      fixed_area.validate();
      emitRow("sabit_alan", fixed_area, repetitions);
    }

    for (const int n : swarm_bt_core::extendedScaleValues()) {
      emitRow(
        "orantili_alan",
        swarm_bt_core::withProportionalArea(base, n, reference), repetitions);
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
