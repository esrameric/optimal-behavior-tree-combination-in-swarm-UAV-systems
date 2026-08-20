// Uctan uca koşu: kurulum, dongu, karsilasma islemesi ve metrik toplama.
#include <gtest/gtest.h>

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
  EXPECT_GT(metrics.encounters, 0);
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

TEST(EpisodeRunner, BesDroneUcDroneDenHizliBitirir)
{
  ExperimentConfig config;
  config.n_agents = 3;
  const auto three = EpisodeRunner(config, 0).run();
  config.n_agents = 5;
  const auto five = EpisodeRunner(config, 0).run();

  EXPECT_LT(five.mission_time, three.mission_time);
  EXPECT_GT(five.encounters, three.encounters);
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
  const auto healthy = EpisodeRunner(config, 0).run();

  config.failure.enabled = true;
  config.failure.time = -1.0;
  config.failure.agent_id = 1;
  const auto degraded = EpisodeRunner(config, 0).run();

  EXPECT_GT(degraded.mission_time, healthy.mission_time);
}

TEST(EpisodeRunner, ChurnOraniSifirIleBirArasinda)
{
  ExperimentConfig config;
  config.failure.enabled = true;
  config.failure.time = -1.0;
  const auto metrics = EpisodeRunner(config, 2).run();

  EXPECT_GE(metrics.churn_ratio, 0.0);
  EXPECT_LE(metrics.churn_ratio, 1.0);
  EXPECT_LE(metrics.churn_events, metrics.encounters);
}

TEST(EpisodeRunner, GecersizKonfigurasyonReddedilir)
{
  ExperimentConfig config;
  config.n_agents = 0;
  EXPECT_THROW(EpisodeRunner(config, 0), std::invalid_argument);
}
