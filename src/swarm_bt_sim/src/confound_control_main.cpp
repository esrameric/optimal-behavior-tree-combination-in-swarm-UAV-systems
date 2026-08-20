// Confound kontrol deneyi - plan Bolum 9.
//
// Plan: "N=5'te daha fazla karsilasma olmasi hem N'in DOGRUDAN etkisi hem de
// yogunluk artisinin DOLAYLI etkisi -- raporda bu ikisini ayirt etmeye calis
// (karsilasma sikligini sabitleyip N'i degistiren bir kontrol deneyi zor ama
// dusunmeye deger)."
//
// Bu arac o kontrol deneyini KOŞAR. Fikir: r_comm, karsilasma sikligini
// dogrudan belirliyor (Bolum 1 kalibrasyonu bunu olctu). Oyleyse her N icin
// r_comm'u, karsilasma sikligi HEDEF bir degere gelecek sekilde ayarlarsak,
// N=3 ile N=5 arasindaki kalan fark yalnizca N'in dogrudan etkisidir.
//
// Uc kol koşulur:
//   serbest  : r_comm sabit (60 m) -- karsilasma sikligi N ile artar (mevcut kurulum)
//   esitli   : r_comm her N icin ayri ayarlanir -- karsilasma sikligi sabit
//   fark     : ikisinin farki = yogunluk artisinin dolayli etkisi
#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_core/parameter_space.hpp>

#include "swarm_bt_sim/episode_runner.hpp"

namespace
{

struct Aggregate
{
  double encounters{0.0};
  double mission_time{0.0};
  double assignment_stability{0.0};
  double churn_ratio{0.0};
  double communication{0.0};
  double swaps{0.0};
};

Aggregate runPoint(const swarm_bt_core::ExperimentConfig & config, int repetitions)
{
  Aggregate result;
  for (int seed = 0; seed < repetitions; ++seed) {
    const auto metrics = swarm_bt_sim::EpisodeRunner(config, seed).run();
    result.encounters += metrics.encounters;
    result.mission_time += metrics.mission_time;
    result.assignment_stability += metrics.assignment_stability;
    result.churn_ratio += metrics.churn_ratio;
    result.communication += metrics.coordination_messages + metrics.shared_cell_updates;
    result.swaps += metrics.swaps;
  }
  const double n = repetitions;
  result.encounters /= n;
  result.mission_time /= n;
  result.assignment_stability /= n;
  result.churn_ratio /= n;
  result.communication /= n;
  result.swaps /= n;
  return result;
}

}  // namespace

int main(int argc, char ** argv)
{
  int repetitions = 10;
  // Esitli kolun r_comm degerleri. Bunlar ARAMAYLA degil, Bolum 1'de zaten
  // olculmus r_comm kalibrasyon tablosundan okunur (calibration_rcomm_bt.csv):
  // N=3, r_comm=60 m -> 2.4 karsilasma; N=5 icin ayni sikliga en yakin deger
  // r_comm=20 m -> 2.3 karsilasma. Aramayi tekrar koşmak hem gereksiz hem de
  // kalibrasyondan bagimsiz ikinci bir olcum uretirdi.
  std::map<int, double> matched_ranges = {{3, 60.0}, {5, 20.0}};

  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--repetitions", [&](const std::string & v) {repetitions = std::stoi(v);}},
    {"--matched", [&](const std::string & v) {
        // Bicim: "3=60,5=20"
        matched_ranges.clear();
        std::size_t start = 0;
        while (start < v.size()) {
          const auto comma = v.find(',', start);
          const auto item = v.substr(start, comma - start);
          const auto equals = item.find('=');
          if (equals == std::string::npos) {
            throw std::invalid_argument("--matched bicimi: 3=60,5=20");
          }
          matched_ranges[std::stoi(item.substr(0, equals))] =
            std::stod(item.substr(equals + 1));
          if (comma == std::string::npos) {
            break;
          }
          start = comma + 1;
        }
      }},
  };

  try {
    for (int i = 1; i < argc; ++i) {
      const std::string flag = argv[i];
      if (flag == "--help") {
        std::cout << "Kullanim: " << argv[0]
                  << " [--repetitions <n>] [--matched 3=60,5=20]\n"
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

    auto base = swarm_bt_core::baselineConfig();
    base.failure.enabled = true;
    base.failure.time = -1.0;
    base.failure.agent_id = -1;

    const auto scales = swarm_bt_core::scaleValues();

    // 1) Serbest kol: r_comm sabit, karsilasma sikligi N ile serbest degisir.
    std::map<int, Aggregate> free_arm;
    for (const int n : scales) {
      auto config = base;
      config.n_agents = n;
      free_arm[n] = runPoint(config, repetitions);
      std::cerr << "serbest N=" << n << " karsilasma=" << free_arm[n].encounters << "\n";
    }

    // 2) Esitli kol: her olcek icin r_comm, karsilasma sikligini kucuk olcegin
    //    degerine getirecek sekilde ayarlanmis olarak verilir.
    std::map<int, Aggregate> matched_arm;
    std::map<int, double> matched_range;
    for (const int n : scales) {
      const auto found = matched_ranges.find(n);
      if (found == matched_ranges.end()) {
        throw std::invalid_argument(
                "N=" + std::to_string(n) + " icin esitli r_comm verilmedi (--matched)");
      }
      auto config = base;
      config.n_agents = n;
      config.r_comm = found->second;
      config.sim.safety_radius = std::min(5.0, config.r_comm / 2.0);
      matched_arm[n] = runPoint(config, repetitions);
      matched_range[n] = config.r_comm;
      std::cerr << "esitli N=" << n << " r_comm=" << config.r_comm
                << " karsilasma=" << matched_arm[n].encounters << "\n";
    }

    std::cout << "kol,N,r_comm,karsilasma,gorev_suresi,atama_kararliligi,"
              << "churn_orani,iletisim_yuku,takas,tekrar\n";
    std::cout << std::fixed << std::setprecision(4);

    auto emit = [&](const std::string & arm, int n, double range, const Aggregate & value) {
        std::cout << arm << "," << n << "," << range << "," << value.encounters << ","
                  << value.mission_time << "," << value.assignment_stability << ","
                  << value.churn_ratio << "," << value.communication << ","
                  << value.swaps << "," << repetitions << "\n";
      };

    for (const int n : scales) {
      emit("serbest", n, base.r_comm, free_arm[n]);
    }
    for (const int n : scales) {
      emit("esitli", n, matched_range[n], matched_arm[n]);
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
