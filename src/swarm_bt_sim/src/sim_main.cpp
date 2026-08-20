// Faz 1 hafif kinematik simulatorun komut satiri kosucusu.
//
// Tek bir koşu calistirir ve ozet metrikleri basar. Deney taramalari (Bolum 4)
// bu calistirilabiliri tekrar tekrar cagirir.
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include <swarm_bt_core/encounter_detector.hpp>
#include <swarm_bt_core/mission_area.hpp>
#include <swarm_bt_core/swarm_state.hpp>

#include "swarm_bt_sim/kinematic_sim.hpp"

namespace
{

struct Options
{
  int n_agents{3};
  double area_side{400.0};
  double cell_size{20.0};
  // Baslangic degeri; Bolum 1'de mission alanina gore kalibre edilir.
  double r_comm{60.0};
  double pheromone_decay{0.01};
  swarm_bt_sim::KinematicSimConfig sim;
};

void printUsage(const char * program)
{
  std::cout
    << "Kullanim: " << program << " [secenekler]\n"
    << "  --n <int>          drone sayisi (varsayilan 3)\n"
    << "  --area <m>         kare gorev alani kenar uzunlugu (varsayilan 400)\n"
    << "  --cell <m>         tarama hucresi kenar uzunlugu (varsayilan 20)\n"
    << "  --r-comm <m>       iletisim menzili (varsayilan 60)\n"
    << "  --speed <m/s>      sabit ucus hizi (varsayilan 10)\n"
    << "  --dt <s>           tick suresi (varsayilan 0.1)\n"
    << "  --time-limit <s>   gorev zaman siniri (varsayilan 3000)\n"
    << "  --decay <0..1>     feromon sonumleme orani (varsayilan 0.01)\n"
    << "  --jitter <0..1>    ajan basina hiz sapmasi orani (varsayilan 0.05)\n"
    << "  --seed <int>       rastgelelik tohumu (varsayilan 0)\n"
    << "  --help             bu yardimi goster\n";
}

bool parseArgs(int argc, char ** argv, Options * options)
{
  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--n", [options](const std::string & v) {options->n_agents = std::stoi(v);}},
    {"--area", [options](const std::string & v) {options->area_side = std::stod(v);}},
    {"--cell", [options](const std::string & v) {options->cell_size = std::stod(v);}},
    {"--r-comm", [options](const std::string & v) {options->r_comm = std::stod(v);}},
    {"--speed", [options](const std::string & v) {options->sim.speed = std::stod(v);}},
    {"--dt", [options](const std::string & v) {options->sim.dt = std::stod(v);}},
    {"--time-limit", [options](const std::string & v) {options->sim.time_limit = std::stod(v);}},
    {"--decay", [options](const std::string & v) {options->pheromone_decay = std::stod(v);}},
    {"--jitter", [options](const std::string & v) {options->sim.speed_jitter = std::stod(v);}},
    {"--seed", [options](const std::string & v) {options->sim.seed = std::stoi(v);}},
  };

  for (int i = 1; i < argc; ++i) {
    const std::string flag = argv[i];
    if (flag == "--help") {
      printUsage(argv[0]);
      return false;
    }
    const auto handler = handlers.find(flag);
    if (handler == handlers.end()) {
      std::cerr << "Bilinmeyen secenek: " << flag << "\n";
      return false;
    }
    if (i + 1 >= argc) {
      std::cerr << "Eksik deger: " << flag << "\n";
      return false;
    }
    handler->second(argv[++i]);
  }
  return true;
}

}  // namespace

int main(int argc, char ** argv)
{
  Options options;
  if (!parseArgs(argc, argv, &options)) {
    return 1;
  }

  try {
    const swarm_bt_core::MissionArea area(
      options.area_side, options.area_side, options.cell_size);
    swarm_bt_core::SwarmState state(area, options.n_agents, options.pheromone_decay);
    state.assignEqualStrips();

    swarm_bt_core::EncounterDetector detector(options.r_comm);
    swarm_bt_sim::KinematicSim sim(state, options.sim);

    while (!sim.finished()) {
      sim.step();
      detector.update(state);
    }

    std::cout << std::fixed << std::setprecision(2)
              << "N                    : " << options.n_agents << "\n"
              << "alan                 : " << options.area_side << " m (hucre "
              << options.cell_size << " m, " << area.cellCount() << " hucre)\n"
              << "r_comm               : " << options.r_comm << " m\n"
              << "tick sayisi          : " << sim.tickCount() << "\n"
              << "gorev suresi         : " << state.time() << " s\n"
              << "kapsama tamam        : " << (state.coverageComplete() ? "evet" : "hayir") << "\n"
              << "karsilasma sikligi   : " << detector.totalEncounters() << "\n"
              << "kapsama dengesizligi : " << state.coverageImbalance() << "\n";
    for (const auto & agent : state.agents()) {
      std::cout << "  ajan " << agent.id
                << " kalan=" << state.remainingCells(agent.id)
                << "/" << agent.region.size()
                << " mesafe=" << agent.distance_travelled << " m\n";
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
