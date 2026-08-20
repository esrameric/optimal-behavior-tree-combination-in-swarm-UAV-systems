// OFAT (One-Factor-At-a-Time) taramasi - plan Bolum 4, Asama 1.
//
// Baseline'dan baslar, her parametreyi TEK TEK degistirir ve her kombinasyonu
// HEM N=3 HEM N=5 ile koşar. Cikti, plan Bolum 8'deki deney kayit sablonuna
// uygun bir CSV'dir; her satir bir (kombinasyon, N) cifti ve o cift icin
// tekrarlarin ortalamasidir.
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

/// Bir (kombinasyon, N) cifti icin tekrarlarin ortalamasi.
struct AggregatedRow
{
  std::string experiment_id;
  int n_agents{0};
  std::map<std::string, double> metrics;
};

AggregatedRow runPoint(const swarm_bt_core::ExperimentConfig & config, int repetitions)
{
  std::map<std::string, std::vector<double>> samples;
  for (int seed = 0; seed < repetitions; ++seed) {
    const auto m = swarm_bt_sim::EpisodeRunner(config, seed).run();
    samples["gorev_tamamlama_suresi"].push_back(m.mission_time);
    samples["tick_maliyeti"].push_back(m.ticks);
    samples["toplam_mesafe"].push_back(m.total_distance);
    samples["karsilasma_sikligi"].push_back(m.encounters);
    samples["koordinasyon_karari"].push_back(m.coordination_events);
    samples["iletisim_yuku"].push_back(m.coordination_messages + m.shared_cell_updates);
    samples["takas_sayisi"].push_back(m.swaps);
    samples["teklif_sayisi"].push_back(m.proposals);
    samples["atama_kararliligi"].push_back(m.assignment_stability);
    samples["churn_orani"].push_back(m.churn_ratio);
    samples["kapsama_dengesizligi"].push_back(m.coverage_imbalance);
    samples["devralinan_hucre"].push_back(m.orphan_transfers);
    samples["carpisma_sayisi"].push_back(m.collisions);
    samples["bilgi_kapsamasi"].push_back(m.known_coverage_ratio);
    samples["kapsama_tamam"].push_back(m.coverage_complete ? 1.0 : 0.0);
  }

  AggregatedRow row;
  row.experiment_id = config.experimentId();
  row.n_agents = config.n_agents;
  for (const auto & entry : samples) {
    row.metrics[entry.first] = mean(entry.second);
  }
  row.metrics["gorev_suresi_std"] = stddev(samples["gorev_tamamlama_suresi"]);
  return row;
}

const std::vector<std::string> & metricOrder()
{
  static const std::vector<std::string> kOrder = {
    "gorev_tamamlama_suresi", "gorev_suresi_std", "tick_maliyeti", "toplam_mesafe",
    "karsilasma_sikligi", "koordinasyon_karari", "iletisim_yuku",
    "teklif_sayisi", "takas_sayisi", "devralinan_hucre",
    "atama_kararliligi", "churn_orani", "kapsama_dengesizligi",
    "carpisma_sayisi", "bilgi_kapsamasi", "kapsama_tamam"};
  return kOrder;
}

}  // namespace

int main(int argc, char ** argv)
{
  int repetitions = 10;
  bool with_failure = true;
  std::string phase = "faz1";

  const std::map<std::string, std::function<void(const std::string &)>> handlers = {
    {"--repetitions", [&](const std::string & v) {repetitions = std::stoi(v);}},
    {"--phase", [&](const std::string & v) {phase = v;}},
  };

  try {
    for (int i = 1; i < argc; ++i) {
      const std::string flag = argv[i];
      if (flag == "--help") {
        std::cout << "Kullanim: " << argv[0]
                  << " [--repetitions <n>] [--phase <ad>] [--no-failure]\n"
                  << "CSV stdout'a yazilir (plan Bolum 8 sablonu).\n";
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

    // Plan Bolum 5/Faz 1: kombinasyon x olcek basina >= 10 tekrar. Koşular
    // rastgele kalkis konumlarindan ve hiz sapmasindan etkilendigi icin tek
    // tohum yaniltici olur.
    constexpr int kMinRepetitions = 10;
    if (repetitions < kMinRepetitions) {
      std::cerr << "UYARI: tekrar sayisi " << repetitions << " < " << kMinRepetitions
                << ". Plan Bolum 5/Faz 1 en az " << kMinRepetitions
                << " tekrar istiyor; sonuclar yayinlanabilir degil.\n";
    }

    auto baseline = swarm_bt_core::baselineConfig();
    baseline.failure.enabled = with_failure;
    baseline.failure.time = -1.0;    // gorev ortasi
    baseline.failure.agent_id = -1;  // rastgele

    const auto variants =
      swarm_bt_core::withScaleVariants(swarm_bt_core::ofatVariants(baseline));

    // Plan Bolum 8 kayit sablonu + bu calismaya ozgu sutunlar.
    std::cout << "deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar";
    for (const auto & metric : metricOrder()) {
      std::cout << "," << metric;
    }
    std::cout << ",notlar\n";
    std::cout << std::fixed << std::setprecision(4);

    for (const auto & config : variants) {
      const auto row = runPoint(config, repetitions);
      std::cout << row.experiment_id << ","
                << swarm_bt_core::combinationId(config) << ","
                << phase << "," << config.n_agents << ","
                << toLetter(config.p2) << "," << toLetter(config.p3) << ","
                << toLetter(config.p4) << "," << config.p5.toLetters() << ","
                << toLetter(config.p6) << "," << repetitions;
      for (const auto & metric : metricOrder()) {
        std::cout << "," << row.metrics.at(metric);
      }
      std::cout << ",\n";
      std::cerr << "tamamlandi: " << row.experiment_id << "\n";
    }
  } catch (const std::exception & error) {
    std::cerr << "Hata: " << error.what() << "\n";
    return 1;
  }
  return 0;
}
