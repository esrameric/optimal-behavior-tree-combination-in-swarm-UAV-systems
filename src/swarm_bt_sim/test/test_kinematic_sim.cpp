#include <gtest/gtest.h>

#include <vector>

#include <swarm_bt_core/encounter_detector.hpp>
#include <swarm_bt_core/mission_area.hpp>
#include <swarm_bt_core/swarm_state.hpp>

#include "swarm_bt_sim/kinematic_sim.hpp"

using swarm_bt_core::MissionArea;
using swarm_bt_core::SwarmState;
using swarm_bt_core::Vec2;
using swarm_bt_sim::KinematicSim;
using swarm_bt_sim::KinematicSimConfig;

namespace
{

SwarmState makeState(int n, double decay = 0.01)
{
  SwarmState state(MissionArea(400.0, 400.0, 20.0), n, decay);
  state.assignEqualStrips();
  return state;
}

/// Koşuyu bitene kadar (veya tick tavanina kadar) ilerletir.
int runToCompletion(KinematicSim * sim, int max_ticks = 200000)
{
  int ticks = 0;
  while (!sim->finished() && ticks < max_ticks) {
    sim->step();
    ++ticks;
  }
  return ticks;
}

}  // namespace

TEST(KinematicSim, GecersizParametrelerReddedilir)
{
  auto state = makeState(3);
  KinematicSimConfig config;
  config.speed = 0.0;
  EXPECT_THROW(KinematicSim(state, config), std::invalid_argument);
  config.speed = 10.0;
  config.dt = -0.1;
  EXPECT_THROW(KinematicSim(state, config), std::invalid_argument);
}

TEST(KinematicSim, SapmasizDurumdaTamOlarakNominalHizlaIlerler)
{
  // Kapsam disi olan ivme modellenmedigi icin, hiz sapmasi kapaliyken adim
  // uzunlugu tam olarak speed*dt olmali.
  auto state = makeState(1);
  KinematicSimConfig config;
  config.speed = 10.0;
  config.dt = 0.1;
  config.speed_jitter = 0.0;
  KinematicSim sim(state, config);

  // Ilk waypoint zaten ajanin altinda: ilk tick onu tarar, sonraki tickler yol alir.
  sim.step();
  const double before = state.agent(0).distance_travelled;
  sim.step();
  EXPECT_NEAR(state.agent(0).distance_travelled - before, config.speed * config.dt, 1e-9);
}

TEST(KinematicSim, HizSapmasiAjanBasinaBandIcindeKalir)
{
  auto state = makeState(5);
  KinematicSimConfig config;
  config.speed = 10.0;
  config.dt = 0.1;
  config.speed_jitter = 0.05;
  config.seed = 3;
  KinematicSim sim(state, config);

  sim.step();
  std::vector<double> before;
  for (const auto & agent : state.agents()) {
    before.push_back(agent.distance_travelled);
  }
  sim.step();

  const double nominal = config.speed * config.dt;
  for (const auto & agent : state.agents()) {
    const double step = agent.distance_travelled - before[static_cast<std::size_t>(agent.id)];
    EXPECT_GE(step, nominal * (1.0 - config.speed_jitter) - 1e-9) << "ajan " << agent.id;
    EXPECT_LE(step, nominal * (1.0 + config.speed_jitter) + 1e-9) << "ajan " << agent.id;
  }
}

TEST(KinematicSim, AyniTohumAyniKoşuyuUretir)
{
  // Tekrarlanabilirlik: plan Bolum 5/Faz 1 kombinasyon basina >= 10 tekrar
  // isterken, tekrarlarin tohumdan turetilmesi sarttir.
  auto run = [](int seed) {
      auto state = makeState(5);
      KinematicSimConfig config;
      config.speed_jitter = 0.05;
      config.seed = seed;
      KinematicSim sim(state, config);
      runToCompletion(&sim);
      return state.time();
    };

  EXPECT_DOUBLE_EQ(run(7), run(7));
  EXPECT_NE(run(7), run(8));
}

TEST(KinematicSim, GecersizHizSapmasiReddedilir)
{
  auto state = makeState(3);
  KinematicSimConfig config;
  config.speed_jitter = -0.1;
  EXPECT_THROW(KinematicSim(state, config), std::invalid_argument);
  config.speed_jitter = 1.0;
  EXPECT_THROW(KinematicSim(state, config), std::invalid_argument);
}

TEST(KinematicSim, WaypointeVarincaHucreZiyaretEdilirVeFeromonBirakilir)
{
  auto state = makeState(1, 0.0);
  const int first_cell = state.agent(0).region.front();
  EXPECT_FALSE(state.isVisited(first_cell));

  KinematicSim sim(state, KinematicSimConfig{});
  sim.step();

  EXPECT_TRUE(state.isVisited(first_cell));
  EXPECT_GT(state.interest().at(first_cell), 0.0);
  EXPECT_EQ(state.agent(0).next_waypoint, 1u);
}

TEST(KinematicSim, FeromonHerTickSonumlenir)
{
  auto state = makeState(1, 0.5);
  KinematicSim sim(state, KinematicSimConfig{});
  sim.step();
  const double after_deposit = state.interest().total();
  ASSERT_GT(after_deposit, 0.0);

  const double before = state.interest().total();
  sim.step();
  // Ikinci tick'te yeni hucreye varilmadi (mesafe > speed*dt), sadece sonumleme oldu.
  EXPECT_LT(state.interest().total(), before);
}

TEST(KinematicSim, KapsamaTamamlanincaKoşuBiter)
{
  for (const int n : {3, 5}) {
    auto state = makeState(n);
    KinematicSim sim(state, KinematicSimConfig{});
    const int ticks = runToCompletion(&sim);

    EXPECT_TRUE(state.coverageComplete()) << "N=" << n;
    EXPECT_LT(state.time(), sim.config().time_limit) << "N=" << n;
    EXPECT_GT(ticks, 0);
    for (const auto & agent : state.agents()) {
      EXPECT_EQ(state.remainingCells(agent.id), 0) << "N=" << n << " ajan " << agent.id;
    }
  }
}

TEST(KinematicSim, DahaFazlaDroneDahaHizliKapsar)
{
  // Alan sabit tutuldugu icin (plan Bolum 1) N=5, N=3'ten hizli bitirmeli.
  auto state3 = makeState(3);
  KinematicSim sim3(state3, KinematicSimConfig{});
  runToCompletion(&sim3);

  auto state5 = makeState(5);
  KinematicSim sim5(state5, KinematicSimConfig{});
  runToCompletion(&sim5);

  EXPECT_LT(state5.time(), state3.time());
}

TEST(KinematicSim, ArizaliAjanHareketEtmez)
{
  // Bolum 2.3 surpriz olayi: BT FAILURE'a zorlanan drone yerinde kalir ve
  // kalan alani taranmamis kalir (devralma mantigi Bolum 2.2'de eklenecek).
  auto state = makeState(3);
  state.agent(1).alive = false;
  const Vec2 frozen = state.agent(1).position;

  KinematicSim sim(state, KinematicSimConfig{});
  for (int i = 0; i < 500; ++i) {
    sim.step();
  }

  EXPECT_DOUBLE_EQ(state.agent(1).position.x, frozen.x);
  EXPECT_DOUBLE_EQ(state.agent(1).position.y, frozen.y);
  EXPECT_DOUBLE_EQ(state.agent(1).distance_travelled, 0.0);
  EXPECT_GT(state.remainingCells(1), 0);
  EXPECT_FALSE(state.coverageComplete());
}

TEST(KinematicSim, BaskasininTaradigiHucreAtlanir)
{
  // Stigmerji kazanci: paylasilan "ziyaret edildi" haritasi tekrar taramayi onler.
  auto state = makeState(2);
  auto & agent = state.agent(0);
  const int second_cell = agent.region[1];
  state.markVisited(second_cell);

  KinematicSim sim(state, KinematicSimConfig{});
  sim.step();  // ilk hucreyi tarar, next_waypoint -> 1

  // Sonraki hedef 1. hucre degil, 2. hucre olmali.
  const int third_cell = agent.region[2];
  const auto target = state.area().cellCenter(third_cell);
  for (int i = 0; i < 100 && !state.isVisited(third_cell); ++i) {
    sim.step();
  }
  EXPECT_TRUE(state.isVisited(third_cell));
  EXPECT_DOUBLE_EQ(agent.position.x, target.x);
  EXPECT_DOUBLE_EQ(agent.position.y, target.y);
}

TEST(KinematicSim, ZamanSiniriKoşuyuSonlandirir)
{
  auto state = makeState(1);
  KinematicSimConfig config;
  config.time_limit = 1.0;
  KinematicSim sim(state, config);

  const int ticks = runToCompletion(&sim);
  EXPECT_FALSE(state.coverageComplete());
  EXPECT_GE(state.time(), config.time_limit);
  EXPECT_EQ(ticks, 10);  // 1.0 s / 0.1 s
}

TEST(KinematicSim, BesDroneUcDroneDenFazlaKarsilasmaUretir)
{
  // Calismanin temel dayanagi (plan Bolum 1): alan sabitken N=5 daha yogun,
  // dolayisiyla comm-range giris olaylari daha sik olmali.
  auto count_encounters = [](int n) {
      auto state = makeState(n);
      swarm_bt_core::EncounterDetector detector(60.0);
      KinematicSim sim(state, KinematicSimConfig{});
      while (!sim.finished()) {
        sim.step();
        detector.update(state);
      }
      return detector.totalEncounters();
    };

  EXPECT_GT(count_encounters(5), count_encounters(3));
}
