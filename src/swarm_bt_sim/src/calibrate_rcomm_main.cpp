// r_comm (iletisim menzili) kalibrasyon taramasi - plan Bolum 1.
//
// Gorev alani sabit tutulurken r_comm degistirilir ve her deger icin N=3 ve N=5
// koşulur; cikti CSV olarak yazilir. Amac, plandaki "karsilasmalarin cok seyrek
// ya da cok sik olmamasi" olcutunu saglayan bir degeri veriyle secmek.
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include <swarm_bt_core/encounter_detector.hpp>
#include <swarm_bt_core/mission_area.hpp>
#include <swarm_bt_core/swarm_state.hpp>

#include "swarm_bt_sim/kinematic_sim.hpp"

namespace
{

struct RunResult
{
  int encounters{0};
  double mission_time{0.0};
  bool coverage_complete{false};
};

RunResult runOnce(
  double area_side, double cell_size, int n_agents, double r_comm,
  double jitter, int seed)
{
  const swarm_bt_core::MissionArea area(area_side, area_side, cell_size);
  swarm_bt_core::SwarmState state(area, n_agents, 0.01);
  state.assignEqualStrips();

  swarm_bt_sim::KinematicSimConfig config;
  config.speed_jitter = jitter;
  config.seed = seed;

  swarm_bt_core::EncounterDetector detector(r_comm);
  swarm_bt_sim::KinematicSim sim(state, config);

  while (!sim.finished()) {
    sim.step();
    detector.update(state);
  }

  return RunResult{detector.totalEncounters(), state.time(), state.coverageComplete()};
}

double mean(const std::vector<double> & values)
{
  if (values.empty()) {
    return 0.0;
  }
  return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

double stddev(const std::vector<double> & values)
{
  if (values.size() < 2) {
    return 0.0;
  }
  const double m = mean(values);
  double sum = 0.0;
  for (const double value : values) {
    sum += (value - m) * (value - m);
  }
  return std::sqrt(sum / static_cast<double>(values.size()));
}

}  // namespace

int main(int argc, char ** argv)
{
  double area_side = 400.0;
  double cell_size = 20.0;
  double jitter = 0.05;
  int repetitions = 10;
  std::vector<int> agent_counts = {3, 5};

  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--area", [&](const std::string & v) {area_side = std::stod(v);}},
    {"--cell", [&](const std::string & v) {cell_size = std::stod(v);}},
    {"--jitter", [&](const std::string & v) {jitter = std::stod(v);}},
    {"--repetitions", [&](const std::string & v) {repetitions = std::stoi(v);}},
  };

  try {
    for (int i = 1; i < argc; ++i) {
      const std::string flag = argv[i];
      if (flag == "--help") {
        std::cout
          << "Kullanim: " << argv[0]
          << " [--area <m>] [--cell <m>] [--jitter <0..1>] [--repetitions <n>]\n"
          << "CSV stdout'a yazilir.\n";
        return 0;
      }
      const auto handler = handlers.find(flag);
      if (handler == handlers.end() || i + 1 >= argc) {
        std::cerr << "Gecersiz secenek: " << flag << "\n";
        return 1;
      }
      handler->second(argv[++i]);
    }

    std::cout << "r_comm,r_comm_orani,N,karsilasma_ort,karsilasma_std,"
              << "gorev_suresi_ort,kapsama_tamam_orani,tekrar\n";
    std::cout << std::fixed << std::setprecision(4);

    // Alan kenar uzunlugunun %2.5'inden %50'sine kadar tara; plandaki oneri
    // araligi (%10-20) bunun icinde kalir, disina da bakip egriyi gorurüz.
    for (int percent = 25; percent <= 500; percent += 25) {
      const double ratio = percent / 1000.0;
      const double r_comm = area_side * ratio;
      for (const int n : agent_counts) {
        std::vector<double> encounters;
        std::vector<double> times;
        int completed = 0;
        for (int seed = 0; seed < repetitions; ++seed) {
          const auto result = runOnce(area_side, cell_size, n, r_comm, jitter, seed);
          encounters.push_back(result.encounters);
          times.push_back(result.mission_time);
          completed += result.coverage_complete ? 1 : 0;
        }
        std::cout << r_comm << "," << ratio << "," << n << ","
                  << mean(encounters) << "," << stddev(encounters) << ","
                  << mean(times) << ","
                  << static_cast<double>(completed) / repetitions << ","
                  << repetitions << "\n";
      }
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
