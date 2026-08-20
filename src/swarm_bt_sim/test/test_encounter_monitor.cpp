// Plan Bolum 7: karsilasma tespiti node'unun cekirdegi.
//
// Kritik iddia: "hem Faz 1 hem Faz 2'de AYNI node kodu kullanilmali, sadece
// pozisyon verisinin kaynagi degisir". Testler bunu, izleyicinin ciktisini
// Faz 1'in dogrudan kullandigi EncounterDetector'in ciktisiyla karsilastirarak
// sinar.
#include <gtest/gtest.h>

#include <vector>

#include <swarm_bt_core/encounter_detector.hpp>
#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_core/swarm_state.hpp>

#include "swarm_bt_sim/encounter_monitor.hpp"

using swarm_bt_core::EncounterDetector;
using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::SwarmState;
using swarm_bt_core::Vec2;
using swarm_bt_sim::EncounterMonitor;

namespace
{

ExperimentConfig makeConfig(int n = 3)
{
  ExperimentConfig config;
  config.n_agents = n;
  config.r_comm = 60.0;
  return config;
}

/// Iki drone'un yaklasip uzaklastigi basit bir yorunge.
std::vector<std::pair<double, double>> approachTrajectory()
{
  std::vector<std::pair<double, double>> steps;
  for (double x = 200.0; x > 20.0; x -= 10.0) {
    steps.emplace_back(0.0, x);      // (a_x, b_x)
  }
  for (double x = 20.0; x < 200.0; x += 10.0) {
    steps.emplace_back(0.0, x);
  }
  return steps;
}

}  // namespace

TEST(EncounterMonitor, GecersizAjanSayisiReddedilir)
{
  EXPECT_THROW(EncounterMonitor(0, makeConfig()), std::invalid_argument);
}

TEST(EncounterMonitor, MenzilDisindaOlayYok)
{
  EncounterMonitor monitor(2, makeConfig(2));
  monitor.setPosition(0, 0.0, 0.0);
  monitor.setPosition(1, 300.0, 0.0);
  EXPECT_TRUE(monitor.update(0.1).empty());
  EXPECT_EQ(monitor.totalEncounters(), 0);
}

TEST(EncounterMonitor, MenzileGirisOlayUretir)
{
  EncounterMonitor monitor(2, makeConfig(2));
  monitor.setPosition(0, 0.0, 0.0);
  monitor.setPosition(1, 40.0, 0.0);

  const auto events = monitor.update(0.1);
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].agent_a, 0);
  EXPECT_EQ(events[0].agent_b, 1);
  EXPECT_DOUBLE_EQ(events[0].distance, 40.0);
}

TEST(EncounterMonitor, ArizaliDroneKarsilasmaUretmez)
{
  EncounterMonitor monitor(2, makeConfig(2));
  monitor.setPosition(0, 0.0, 0.0);
  monitor.setPosition(1, 40.0, 0.0);
  monitor.setAlive(1, false);
  EXPECT_TRUE(monitor.update(0.1).empty());
}

TEST(EncounterMonitor, FazlarArasiCiktiAYNI)
{
  // Ayni yorunge, iki farkli yol: (1) izleyici uzerinden (Faz 2 node'unun
  // kullandigi yol), (2) dogrudan EncounterDetector (Faz 1 koşu dongusunun
  // kullandigi yol). Sonuclar birebir ayni olmali.
  const auto config = makeConfig(2);

  EncounterMonitor monitor(2, config);
  SwarmState state(config.missionArea(), 2, 0.0);
  EncounterDetector detector(config.r_comm, config.encounter_hysteresis);

  int monitor_events = 0;
  int detector_events = 0;
  double time = 0.0;

  for (const auto & step : approachTrajectory()) {
    time += config.sim.dt;

    monitor.setPosition(0, step.first, 0.0);
    monitor.setPosition(1, step.second, 0.0);
    monitor_events += static_cast<int>(monitor.update(time).size());

    state.agent(0).position = Vec2{step.first, 0.0};
    state.agent(1).position = Vec2{step.second, 0.0};
    state.setTime(time);
    detector_events += static_cast<int>(detector.update(state).size());
  }

  EXPECT_EQ(monitor_events, detector_events);
  EXPECT_EQ(monitor.totalEncounters(), detector.totalEncounters());
  EXPECT_GT(monitor.totalEncounters(), 0) << "yorunge hic karsilasma uretmedi";
}

TEST(EncounterMonitor, HisterezisTekYakinlasmayiTekOlaySayar)
{
  EncounterMonitor monitor(2, makeConfig(2));
  monitor.setPosition(0, 0.0, 0.0);

  // Esigin iki yaninda salin: histerezis sayesinde tek olay olmali.
  double time = 0.0;
  for (int i = 0; i < 20; ++i) {
    time += 0.1;
    monitor.setPosition(1, (i % 2 == 0) ? 59.0 : 61.0, 0.0);
    monitor.update(time);
  }
  EXPECT_EQ(monitor.totalEncounters(), 1);
}

TEST(EncounterMonitor, HerTickModelindeHerCagriKontrolEder)
{
  auto config = makeConfig(2);
  config.p6 = swarm_bt_core::TriggerModel::kEveryTick;
  EncounterMonitor monitor(2, config);
  monitor.setPosition(0, 0.0, 0.0);
  monitor.setPosition(1, 300.0, 0.0);

  for (int i = 1; i <= 10; ++i) {
    monitor.update(i * 0.1);
  }
  EXPECT_EQ(monitor.checkCount(), 10);
}

TEST(EncounterMonitor, PeriyodikYoklamaDahaAzKontrolEder)
{
  // P6a ucuz ama kayipli: iki yoklama arasinda girip cikan cift gorulmez.
  auto config = makeConfig(2);
  config.p6 = swarm_bt_core::TriggerModel::kPeriodicPolling;
  config.sim.poll_period = 1.0;
  EncounterMonitor monitor(2, config);
  monitor.setPosition(0, 0.0, 0.0);
  monitor.setPosition(1, 300.0, 0.0);

  for (int i = 1; i <= 50; ++i) {   // 5 saniye, dt = 0.1
    monitor.update(i * 0.1);
  }
  EXPECT_LT(monitor.checkCount(), 50);
  EXPECT_GE(monitor.checkCount(), 5);
}

TEST(EncounterMonitor, PeriyodikYoklamaAradakiGirisiKacirabilir)
{
  auto config = makeConfig(2);
  config.p6 = swarm_bt_core::TriggerModel::kPeriodicPolling;
  config.sim.poll_period = 5.0;
  EncounterMonitor monitor(2, config);

  monitor.setPosition(0, 0.0, 0.0);
  monitor.setPosition(1, 300.0, 0.0);
  monitor.update(0.1);              // ilk yoklama

  // Yaklas ve uzaklas: yoklama araligi icinde kaldigi icin gorulmemeli.
  monitor.setPosition(1, 30.0, 0.0);
  monitor.update(1.0);
  monitor.setPosition(1, 300.0, 0.0);
  monitor.update(2.0);

  EXPECT_EQ(monitor.totalEncounters(), 0) << "yoklama arasindaki giris gorulmemeliydi";
}
