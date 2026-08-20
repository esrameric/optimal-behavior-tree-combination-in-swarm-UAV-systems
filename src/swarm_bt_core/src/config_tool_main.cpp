// experiment_<deney_id>.yaml deney config dosyalari ureten arac.
//
// Plan Bolum 0 "Konfigurasyon Yonetimi": her deney icin bir YAML dosyasi.
// Dosya adi, Bolum 3'teki deney kimligi semasindan turetilir.
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>

#include "swarm_bt_core/experiment_config.hpp"

namespace
{

void printUsage(const char * program)
{
  std::cout
    << "Kullanim: " << program << " [secenekler]\n"
    << "  --p2 <a|b|c>       koordinasyon mimarisi (varsayilan c)\n"
    << "  --p3 <a|b|c>       baslangic alan atama (varsayilan c)\n"
    << "  --p4 <a|b|c>       BT mimarisi (varsayilan b)\n"
    << "  --p5 <harfler>     iletisim mekanizmalari, orn. abc (varsayilan abc)\n"
    << "  --p6 <a|b|c>       tetikleme modeli (varsayilan c)\n"
    << "  --n <int>          drone sayisi (varsayilan 3)\n"
    << "  --r-comm <m>       iletisim menzili (varsayilan 60)\n"
    << "  --esik <0..1>      takas tetikleme esigi (varsayilan 0.30)\n"
    << "  --area <m>         gorev alani kenar uzunlugu (varsayilan 400)\n"
    << "  --repetitions <n>  tekrar sayisi (varsayilan 10)\n"
    << "  --seed <int>       rastgelelik tohumu (varsayilan 0)\n"
    << "  --id <deney_id>    tum parametre uzayini kimlikten al, orn.\n"
    << "                     P2b_P3c_P4c_P5bc_P6c_N3\n"
    << "  --out-dir <dizin>  cikti dizini; verilmezse YAML stdout'a yazilir\n"
    << "  --help             bu yardimi goster\n";
}

}  // namespace

int main(int argc, char ** argv)
{
  swarm_bt_core::ExperimentConfig config;
  std::string out_dir;

  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--p2", [&](const std::string & v) {config.p2 = swarm_bt_core::coordinationFromLetter(v);}},
    {"--p3", [&](const std::string & v) {config.p3 = swarm_bt_core::allocationFromLetter(v);}},
    {"--p4", [&](const std::string & v) {config.p4 = swarm_bt_core::btArchitectureFromLetter(v);}},
    {"--p5", [&](const std::string & v) {
        config.p5 = swarm_bt_core::CommunicationMechanisms::fromLetters(v);
      }},
    {"--p6", [&](const std::string & v) {config.p6 = swarm_bt_core::triggerModelFromLetter(v);}},
    {"--n", [&](const std::string & v) {config.n_agents = std::stoi(v);}},
    {"--r-comm", [&](const std::string & v) {config.r_comm = std::stod(v);}},
    {"--esik", [&](const std::string & v) {config.swap_threshold = std::stod(v);}},
    {"--area", [&](const std::string & v) {config.sim.area_side = std::stod(v);}},
    {"--repetitions", [&](const std::string & v) {config.repetitions = std::stoi(v);}},
    {"--seed", [&](const std::string & v) {config.seed = std::stoi(v);}},
    {"--id", [&](const std::string & v) {
        config = swarm_bt_core::ExperimentConfig::fromExperimentId(v, config);
      }},
    {"--out-dir", [&](const std::string & v) {out_dir = v;}},
  };

  try {
    for (int i = 1; i < argc; ++i) {
      const std::string flag = argv[i];
      if (flag == "--help") {
        printUsage(argv[0]);
        return 0;
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
    const std::string yaml = config.toYaml();

    if (out_dir.empty()) {
      std::cout << yaml << "\n";
      return 0;
    }

    std::filesystem::create_directories(out_dir);
    const auto path =
      std::filesystem::path(out_dir) / ("experiment_" + config.experimentId() + ".yaml");
    std::ofstream file(path);
    if (!file) {
      std::cerr << "Dosya yazilamadi: " << path << "\n";
      return 1;
    }
    file << yaml << "\n";
    std::cout << path.string() << "\n";
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
