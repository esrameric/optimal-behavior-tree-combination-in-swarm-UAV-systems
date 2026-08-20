// r_comm (iletisim menzili) kalibrasyon taramasi - plan Bolum 1 ve Bolum 5/Faz 0.
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
  int repetitions = 10;
  bool with_failure = true;
  auto base = swarm_bt_core::baselineConfig();

  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--repetitions", [&](const std::string & v) {repetitions = std::stoi(v);}},
    {"--area", [&](const std::string & v) {base.sim.area_side = std::stod(v);}},
    {"--cell", [&](const std::string & v) {base.sim.cell_size = std::stod(v);}},
    {"--jitter", [&](const std::string & v) {base.sim.speed_jitter = std::stod(v);}},
  };

  try {
    for (int i = 1; i < argc; ++i) {
      const std::string flag = argv[i];
      if (flag == "--help") {
        std::cout << "Kullanim: " << argv[0]
                  << " [--repetitions <n>] [--area <m>] [--cell <m>] [--jitter <0..1>]"
                  << " [--no-failure]\nCSV stdout'a yazilir.\n";
        return 0;
      }
      if (flag == "--no-failure") {
        with_failure = false;
        continue;
      }
      const auto handler = handlers.find(flag);
      if (handler == handlers.end() || i + 1 >= argc) {
        std::cerr << "Gecersiz secenek: " << flag << "\n";
        return 1;
      }
      handler->second(argv[++i]);
    }

    base.failure.enabled = with_failure;
    base.failure.time = -1.0;
    base.failure.agent_id = -1;

    std::cout << "r_comm,r_comm_orani,N,karsilasma_ort,karsilasma_std,"
              << "koordinasyon_karari_ort,takas_ort,gorev_suresi_ort,gorev_suresi_std,"
              << "kapsama_tamam_orani,tekrar\n";
    std::cout << std::fixed << std::setprecision(4);

    // Alan kenar uzunlugunun %2.5'inden %50'sine kadar tara; plandaki oneri
    // araligi (%10-20) bunun icinde kalir, disina da bakip egriyi goruruz.
    for (int percent = 25; percent <= 500; percent += 25) {
      const double ratio = percent / 1000.0;
      auto config = base;
      config.r_comm = base.sim.area_side * ratio;
      config.sim.safety_radius = std::min(5.0, config.r_comm / 2.0);

      for (const int n : swarm_bt_core::scaleValues()) {
        config.n_agents = n;
        std::vector<double> encounters;
        std::vector<double> decisions;
        std::vector<double> swaps;
        std::vector<double> times;
        int completed = 0;
        for (int seed = 0; seed < repetitions; ++seed) {
          const auto metrics = swarm_bt_sim::EpisodeRunner(config, seed).run();
          encounters.push_back(metrics.encounters);
          decisions.push_back(metrics.coordination_events);
          swaps.push_back(metrics.swaps);
          times.push_back(metrics.mission_time);
          completed += metrics.coverage_complete ? 1 : 0;
        }
        std::cout << config.r_comm << "," << ratio << "," << n << ","
                  << mean(encounters) << "," << stddev(encounters) << ","
                  << mean(decisions) << "," << mean(swaps) << ","
                  << mean(times) << "," << stddev(times) << ","
                  << static_cast<double>(completed) / repetitions << ","
                  << repetitions << "\n";
      }
      std::cerr << "tamamlandi: r_comm=" << config.r_comm << "\n";
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
