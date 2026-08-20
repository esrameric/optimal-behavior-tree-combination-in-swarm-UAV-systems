// KinematicSim artik yalnizca UCUS KATMANIDIR: hedefi BT secer, bu sinif
// ucurur. Testler bu ayrimi dogrular -- hedef verilmeden ajan hareket etmemeli.
#include <gtest/gtest.h>

#include <vector>

#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_core/mission_area.hpp>
#include <swarm_bt_core/swarm_state.hpp>

#include "swarm_bt_sim/kinematic_sim.hpp"

using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::MissionArea;
using swarm_bt_core::SwarmState;
using swarm_bt_core::Vec2;
using swarm_bt_sim::KinematicSim;
using swarm_bt_sim::KinematicSimConfig;

namespace
{

SwarmState makeState(int n, double decay = 0.01)
{
  ExperimentConfig config;
  config.n_agents = n;
  config.sim.pheromone_decay = decay;
  config.sim.random_launch = false;   // deterministik kalkis: testler sade kalsin
  config.sim.interest_points = 0;
  return swarm_bt_core::makeSwarmState(config, 0);
}

/// BT'nin yerine gecen asgari surucu: her ajana bolgesindeki siradaki
/// taranmamis hucreyi hedef olarak verir.
void assignNextTargets(SwarmState * state)
{
  for (auto & agent : state->agents()) {
    if (!agent.alive) {
      continue;
    }
    if (agent.at_target) {
      agent.at_target = false;
      agent.target_cell = -1;
      ++agent.next_waypoint;
    }
    if (agent.target_cell >= 0) {
      continue;
    }
    for (std::size_t i = agent.next_waypoint; i < agent.region.size(); ++i) {
      if (!state->isVisited(agent.region[i])) {
        agent.next_waypoint = i;
        agent.target_cell = agent.region[i];
        break;
      }
    }
  }
}

int runToCompletion(KinematicSim * sim, SwarmState * state, int max_ticks = 200000)
{
  int ticks = 0;
  while (!sim->finished() && ticks < max_ticks) {
    assignNextTargets(state);
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
  config.dt = 0.1;
  config.speed_jitter = -0.1;
  EXPECT_THROW(KinematicSim(state, config), std::invalid_argument);
  config.speed_jitter = 1.0;
  EXPECT_THROW(KinematicSim(state, config), std::invalid_argument);
}

TEST(KinematicSim, HedefVerilmedenAjanHareketEtmez)
{
  // Ucus katmani kendi basina hedef SECMEZ; bu BT'nin isi.
  auto state = makeState(3);
  const Vec2 start = state.agent(0).position;

  KinematicSim sim(state, KinematicSimConfig{});
  for (int i = 0; i < 100; ++i) {
    sim.step();
  }

  EXPECT_DOUBLE_EQ(state.agent(0).position.x, start.x);
  EXPECT_DOUBLE_EQ(state.agent(0).position.y, start.y);
  EXPECT_DOUBLE_EQ(state.agent(0).distance_travelled, 0.0);
  EXPECT_FALSE(state.isVisited(state.agent(0).region.front()));
}

TEST(KinematicSim, SapmasizDurumdaTamOlarakNominalHizlaIlerler)
{
  auto state = makeState(1);
  KinematicSimConfig config;
  config.speed = 10.0;
  config.dt = 0.1;
  config.speed_jitter = 0.0;
  KinematicSim sim(state, config);

  // Uzak bir hedef ver: birkac tick boyunca yol alsin.
  state.agent(0).target_cell = state.agent(0).region.back();
  state.agent(0).at_target = false;

  sim.step();
  const double first = state.agent(0).distance_travelled;
  sim.step();
  EXPECT_NEAR(first, config.speed * config.dt, 1e-9);
  EXPECT_NEAR(state.agent(0).distance_travelled - first, config.speed * config.dt, 1e-9);
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

  for (auto & agent : state.agents()) {
    agent.target_cell = agent.region.back();
    agent.at_target = false;
  }
  sim.step();

  const double nominal = config.speed * config.dt;
  for (const auto & agent : state.agents()) {
    EXPECT_GE(agent.distance_travelled, nominal * (1.0 - config.speed_jitter) - 1e-9);
    EXPECT_LE(agent.distance_travelled, nominal * (1.0 + config.speed_jitter) + 1e-9);
  }
}

TEST(KinematicSim, HedefeVarincaHucreTaranirVeBildirilir)
{
  auto state = makeState(1, 0.0);
  const int first_cell = state.agent(0).region.front();
  state.agent(0).target_cell = first_cell;

  KinematicSim sim(state, KinematicSimConfig{});
  sim.step();

  EXPECT_TRUE(state.isVisited(first_cell));
  EXPECT_TRUE(state.knowsVisited(0, first_cell));
  EXPECT_TRUE(state.agent(0).at_target) << "ucus katmani varisi BT'ye bildirmeli";
}

TEST(KinematicSim, VarilanHedefteBeklerYeniKomutaKadar)
{
  // at_target isaretliyken ajan hareket etmemeli: BT'nin islemesi bekleniyor.
  auto state = makeState(1, 0.0);
  state.agent(0).target_cell = state.agent(0).region.front();

  KinematicSim sim(state, KinematicSimConfig{});
  sim.step();
  ASSERT_TRUE(state.agent(0).at_target);
  const double travelled = state.agent(0).distance_travelled;

  for (int i = 0; i < 20; ++i) {
    sim.step();
  }
  EXPECT_DOUBLE_EQ(state.agent(0).distance_travelled, travelled);
}

TEST(KinematicSim, FeromonYalnizcaIlgiNoktasindaBirakilir)
{
  auto without_point = makeState(1, 0.0);
  without_point.agent(0).target_cell = without_point.agent(0).region.front();
  KinematicSim sim_without(without_point, KinematicSimConfig{});
  sim_without.step();
  EXPECT_DOUBLE_EQ(without_point.interest().total(), 0.0);

  auto with_point = makeState(1, 0.0);
  const int first_cell = with_point.agent(0).region.front();
  with_point.placeInterestPointsAt({first_cell});
  with_point.agent(0).target_cell = first_cell;
  KinematicSim sim_with(with_point, KinematicSimConfig{});
  sim_with.step();
  EXPECT_GT(with_point.interest().at(first_cell), 0.0);
}

TEST(KinematicSim, FeromonHerTickSonumlenir)
{
  auto state = makeState(1, 0.5);
  const int first_cell = state.agent(0).region.front();
  state.placeInterestPointsAt({first_cell});
  state.agent(0).target_cell = first_cell;

  KinematicSim sim(state, KinematicSimConfig{});
  sim.step();
  const double after_deposit = state.interest().total();
  ASSERT_GT(after_deposit, 0.0);

  sim.step();
  EXPECT_LT(state.interest().total(), after_deposit);
}

TEST(KinematicSim, ArizaliAjanHareketEtmez)
{
  auto state = makeState(3);
  state.agent(1).alive = false;
  state.agent(1).target_cell = state.agent(1).region.front();
  const Vec2 frozen = state.agent(1).position;

  KinematicSim sim(state, KinematicSimConfig{});
  for (int i = 0; i < 100; ++i) {
    sim.step();
  }

  EXPECT_DOUBLE_EQ(state.agent(1).position.x, frozen.x);
  EXPECT_DOUBLE_EQ(state.agent(1).position.y, frozen.y);
  EXPECT_DOUBLE_EQ(state.agent(1).distance_travelled, 0.0);
}

TEST(KinematicSim, HedefSurucusuyleKapsamaTamamlanir)
{
  for (const int n : {3, 5}) {
    auto state = makeState(n);
    KinematicSim sim(state, KinematicSimConfig{});
    runToCompletion(&sim, &state);

    EXPECT_TRUE(state.coverageComplete()) << "N=" << n;
    EXPECT_LT(state.time(), sim.config().time_limit) << "N=" << n;
  }
}

TEST(KinematicSim, DahaFazlaDroneDahaHizliKapsar)
{
  auto state3 = makeState(3);
  KinematicSim sim3(state3, KinematicSimConfig{});
  runToCompletion(&sim3, &state3);

  auto state5 = makeState(5);
  KinematicSim sim5(state5, KinematicSimConfig{});
  runToCompletion(&sim5, &state5);

  EXPECT_LT(state5.time(), state3.time());
}

TEST(KinematicSim, ZamanSiniriKoşuyuSonlandirir)
{
  auto state = makeState(1);
  KinematicSimConfig config;
  config.time_limit = 1.0;
  KinematicSim sim(state, config);

  const int ticks = runToCompletion(&sim, &state);
  EXPECT_FALSE(state.coverageComplete());
  EXPECT_GE(state.time(), config.time_limit);
  EXPECT_EQ(ticks, 10);   // 1.0 s / 0.1 s
}
