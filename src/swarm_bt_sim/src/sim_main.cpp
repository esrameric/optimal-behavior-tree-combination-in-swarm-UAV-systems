// Faz 1 hafif kinematik simulatorun komut satiri kosucusu.
//
// Tek bir koşu calistirir ve ozet metrikleri basar. Konfigurasyon ya bir deney
// YAML dosyasindan (--config) ya da komut satiri seceneklerinden gelir; deney
// taramalari (Bolum 4) bu calistirilabiliri tekrar tekrar cagirir.
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include <swarm_bt_core/experiment_config.hpp>

#include "swarm_bt_sim/episode_runner.hpp"

namespace
{

void printUsage(const char * program)
{
  std::cout
    << "Kullanim: " << program << " [secenekler]\n"
    << "  --config <yaml>       deney config dosyasi (digerleri bunun uzerine yazar)\n"
    << "  --n <int>             drone sayisi\n"
    << "  --r-comm <m>          iletisim menzili\n"
    << "  --esik <0..1>         takas tetikleme esigi\n"
    << "  --area <m>            kare gorev alani kenar uzunlugu\n"
    << "  --cell <m>            tarama hucresi kenar uzunlugu\n"
    << "  --speed <m/s>         nominal ucus hizi\n"
    << "  --jitter <0..1>       ajan basina hiz sapmasi orani\n"
    << "  --dt <s>              tick suresi\n"
    << "  --time-limit <s>      gorev zaman siniri\n"
    << "  --decay <0..1>        feromon sonumleme orani\n"
    << "  --failure             surpriz olay: bir drone'u arizalandir (Bolum 2.3)\n"
    << "  --failure-time <s>    ariza zamani; negatifse gorev ortasi (kapsama %50)\n"
    << "  --failure-agent <id>  arizalanacak drone; negatifse rastgele\n"
    << "  --seed <int>          rastgelelik tohumu\n"
    << "  --print-launch-positions  ajanlarin kalkis konumlarini yazip cik\n"
    << "  --help                bu yardimi goster\n";
}

}  // namespace

int main(int argc, char ** argv)
{
  swarm_bt_core::ExperimentConfig config;
  int seed = 0;
  bool print_launch_positions = false;

  // --config once islenmeli ki diger secenekler onun uzerine yazabilsin.
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == "--config") {
      try {
        config = swarm_bt_core::ExperimentConfig::fromYamlFile(argv[i + 1]);
        seed = config.seed;
      } catch (const std::exception & error) {
        std::cerr << "Hata: " << error.what() << "\n";
        return 1;
      }
    }
  }

  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--config", [](const std::string &) {}},   // yukarida islendi
    {"--n", [&](const std::string & v) {config.n_agents = std::stoi(v);}},
    {"--r-comm", [&](const std::string & v) {config.r_comm = std::stod(v);}},
    {"--esik", [&](const std::string & v) {config.swap_threshold = std::stod(v);}},
    {"--area", [&](const std::string & v) {config.sim.area_side = std::stod(v);}},
    {"--cell", [&](const std::string & v) {config.sim.cell_size = std::stod(v);}},
    {"--speed", [&](const std::string & v) {config.sim.speed = std::stod(v);}},
    {"--jitter", [&](const std::string & v) {config.sim.speed_jitter = std::stod(v);}},
    {"--dt", [&](const std::string & v) {config.sim.dt = std::stod(v);}},
    {"--time-limit", [&](const std::string & v) {config.sim.time_limit = std::stod(v);}},
    {"--decay", [&](const std::string & v) {config.sim.pheromone_decay = std::stod(v);}},
    {"--failure-time", [&](const std::string & v) {
        config.failure.enabled = true;
        config.failure.time = std::stod(v);
      }},
    {"--failure-agent", [&](const std::string & v) {
        config.failure.enabled = true;
        config.failure.agent_id = std::stoi(v);
      }},
    {"--seed", [&](const std::string & v) {seed = std::stoi(v);}},
  };

  try {
    for (int i = 1; i < argc; ++i) {
      const std::string flag = argv[i];
      if (flag == "--help") {
        printUsage(argv[0]);
        return 0;
      }
      if (flag == "--failure") {
        config.failure.enabled = true;
        continue;
      }
      if (flag == "--print-launch-positions") {
        print_launch_positions = true;
        continue;
      }
      const auto handler = handlers.find(flag);
      if (handler == handlers.end()) {
        std::cerr << "Bilinmeyen secenek: " << flag << "\n";
        return 1;
      }
      if (i + 1 >= argc) {
        std::cerr << "Eksik deger: " << flag << "\n";
        return 1;
      }
      handler->second(argv[++i]);
    }

    config.validate();

    if (print_launch_positions) {
      // Faz 2'de Gazebo dunyasi, ajanlari Faz 1 ile AYNI konumlara yerlestirmek
      // icin bu ciktiyi kullanir. Iki fazin ayni kalkis geometrisinden
      // baslamasi, plan Bolum 5/Faz 2'deki karsilastirmanin on kosulu.
      const auto state = swarm_bt_core::makeSwarmState(config, seed);
      for (const auto & agent : state.agents()) {
        std::cout << agent.position.x << " " << agent.position.y << "\n";
      }
      return 0;
    }

    swarm_bt_sim::EpisodeRunner runner(config, seed);
    const auto metrics = runner.run();
    const auto & state = runner.state();

    std::cout << std::fixed << std::setprecision(2)
              << "deney_id             : " << config.experimentId() << "\n"
              << "tohum                : " << seed << "\n"
              << "alan                 : " << config.sim.area_side << " m ("
              << state.area().cellCount() << " hucre, hucre basi "
              << config.sim.cell_size << " m)\n"
              << "r_comm / esik_degeri : " << config.r_comm << " m / "
              << config.swap_threshold << "\n"
              << "-----------------------------------------\n"
              << "gorev suresi         : " << metrics.mission_time << " s\n"
              << "tick maliyeti        : " << metrics.ticks << "\n"
              << "kapsama tamam        : " << (metrics.coverage_complete ? "evet" : "hayir") << "\n"
              << "toplam mesafe        : " << metrics.total_distance << " m\n"
              << "karsilasma sikligi   : " << metrics.encounters << "\n"
              << "koordinasyon karari  : " << metrics.coordination_events << "\n"
              << "yoklama kontrolu     : " << metrics.detection_checks << "\n"
              << "takas teklifi        : " << metrics.proposals << "\n"
              << "kabul edilen takas   : " << metrics.swaps << "\n"
              << "devralinan hucre     : " << metrics.orphan_transfers << "\n"
              << "churn orani          : " << metrics.churn_ratio << "\n"
              << "atama kararliligi    : " << metrics.assignment_stability << "\n"
              << "kapsama dengesizligi : " << metrics.coverage_imbalance << "\n";
    if (metrics.failed_agent >= 0) {
      std::cout << "arizalanan drone     : " << metrics.failed_agent
                << " (t=" << metrics.failure_time << " s)\n";
    }
    for (const auto & agent : state.agents()) {
      std::cout << "  ajan " << agent.id << (agent.alive ? " [canli]  " : " [arizali]")
                << " kalan=" << state.remainingCells(agent.id)
                << "/" << agent.region.size()
                << " mesafe=" << agent.distance_travelled << " m"
                << " atama_degisikligi=" << agent.assignment_changes << "\n";
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
