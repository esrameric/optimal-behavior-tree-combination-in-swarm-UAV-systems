// Uctan uca koşu: kurulum, dongu, karsilasma islemesi ve metrik toplama.
#include <gtest/gtest.h>

#include <set>

#include <swarm_bt_core/experiment_config.hpp>

#include "swarm_bt_sim/episode_runner.hpp"

using swarm_bt_core::ExperimentConfig;
using swarm_bt_sim::EpisodeRunner;

TEST(EpisodeRunner, BaselineKoşusuKapsamayiTamamlar)
{
  ExperimentConfig config;
  EpisodeRunner runner(config, 0);
  const auto metrics = runner.run();

  EXPECT_TRUE(metrics.coverage_complete);
  EXPECT_GT(metrics.ticks, 0);
  EXPECT_GT(metrics.mission_time, 0.0);
  EXPECT_LT(metrics.mission_time, config.sim.time_limit);
  EXPECT_GT(metrics.total_distance, 0.0);
}

TEST(EpisodeRunner, TekrarlarArasindaGercekDegiskenlikVar)
{
  // Kalkis konumlari tohumdan turedigi icin koşular birbirinden farkli olmali;
  // aksi halde plan Bolum 5/Faz 1'in istedigi >=10 tekrar hicbir sey ortalamaz.
  ExperimentConfig config;
  std::set<double> times;
  int total_encounters = 0;
  for (int seed = 0; seed < 10; ++seed) {
    const auto metrics = EpisodeRunner(config, seed).run();
    times.insert(metrics.mission_time);
    total_encounters += metrics.encounters;
    EXPECT_TRUE(metrics.coverage_complete) << "tohum " << seed;
  }
  EXPECT_GT(times.size(), 5u) << "koşular birbirinin ayni cikiyor";
  EXPECT_GT(total_encounters, 0) << "10 koşuda hic karsilasma olmadi";
}

TEST(EpisodeRunner, AyniTohumAyniMetrikleriUretir)
{
  ExperimentConfig config;
  const auto a = EpisodeRunner(config, 5).run();
  const auto b = EpisodeRunner(config, 5).run();

  EXPECT_DOUBLE_EQ(a.mission_time, b.mission_time);
  EXPECT_EQ(a.encounters, b.encounters);
  EXPECT_EQ(a.swaps, b.swaps);
  EXPECT_DOUBLE_EQ(a.total_distance, b.total_distance);
}

TEST(EpisodeRunner, BesDroneUcDroneDenHizliBitirirVeDahaCokKarsilasir)
{
  // Tekil tohumlar gurultulu; ortalama uzerinden bakilir.
  ExperimentConfig config;
  double time_three = 0.0;
  double time_five = 0.0;
  int encounters_three = 0;
  int encounters_five = 0;

  for (int seed = 0; seed < 10; ++seed) {
    config.n_agents = 3;
    const auto three = EpisodeRunner(config, seed).run();
    config.n_agents = 5;
    const auto five = EpisodeRunner(config, seed).run();
    time_three += three.mission_time;
    time_five += five.mission_time;
    encounters_three += three.encounters;
    encounters_five += five.encounters;
  }

  EXPECT_LT(time_five, time_three);
  // Plan Bolum 1'in dayanagi: alan sabitken N=5 daha yogun -> daha sik karsilasma.
  EXPECT_GT(encounters_five, encounters_three);
}

TEST(EpisodeRunner, ArizasizKoşudaDevralmaOlmaz)
{
  ExperimentConfig config;
  config.failure.enabled = false;
  const auto metrics = EpisodeRunner(config, 0).run();

  EXPECT_EQ(metrics.orphan_transfers, 0);
  EXPECT_EQ(metrics.failed_agent, -1);
  EXPECT_TRUE(metrics.coverage_complete);
}

TEST(EpisodeRunner, ArizaTetiklenirVeAlanDevralinir)
{
  // Plan Bolum 2.3: arizalanan drone'un kalan alani devralinmali ve gorev
  // yine de tamamlanabilmeli.
  ExperimentConfig config;
  config.failure.enabled = true;
  config.failure.time = -1.0;    // gorev ortasi
  config.failure.agent_id = 1;   // ortadaki serit
  const auto metrics = EpisodeRunner(config, 0).run();

  EXPECT_EQ(metrics.failed_agent, 1);
  EXPECT_GT(metrics.failure_time, 0.0);
  EXPECT_GT(metrics.orphan_transfers, 0);
  EXPECT_GT(metrics.assignment_stability, 0.0);
}

TEST(EpisodeRunner, ArizaGorevSuresiniUzatir)
{
  ExperimentConfig config;
  double healthy_total = 0.0;
  double degraded_total = 0.0;
  for (int seed = 0; seed < 10; ++seed) {
    healthy_total += EpisodeRunner(config, seed).run().mission_time;

    auto with_failure = config;
    with_failure.failure.enabled = true;
    with_failure.failure.time = -1.0;
    with_failure.failure.agent_id = 1;
    degraded_total += EpisodeRunner(with_failure, seed).run().mission_time;
  }
  EXPECT_GT(degraded_total, healthy_total);
}

TEST(EpisodeRunner, ChurnOraniSifirIleBirArasinda)
{
  // churn orani karsilasma BASINA tanimli: bosta kalan ajanin devralmasi
  // (idle_claims) paya girmemeli, yoksa oran 1'i asabilir.
  ExperimentConfig config;
  config.failure.enabled = true;
  config.failure.time = -1.0;
  for (int seed = 0; seed < 10; ++seed) {
    const auto metrics = EpisodeRunner(config, seed).run();
    EXPECT_GE(metrics.churn_ratio, 0.0) << "tohum " << seed;
    EXPECT_LE(metrics.churn_ratio, 1.0) << "tohum " << seed;
    EXPECT_LE(metrics.churn_events, metrics.encounters) << "tohum " << seed;
  }
}

TEST(EpisodeRunner, GecersizKonfigurasyonReddedilir)
{
  ExperimentConfig config;
  config.n_agents = 0;
  EXPECT_THROW(EpisodeRunner(config, 0), std::invalid_argument);
}
