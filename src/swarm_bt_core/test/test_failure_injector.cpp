// Plan Bolum 2.3: gorev ortasinda bir drone'u arizalandirma ve kalan alaninin
// devralinmasi.
#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "swarm_bt_core/area_swap.hpp"
#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/failure_injector.hpp"

using swarm_bt_core::AreaSwapNegotiator;
using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::FailureInjection;
using swarm_bt_core::FailureInjector;
using swarm_bt_core::SwarmState;
using swarm_bt_core::makeSwarmState;

namespace
{

SwarmState makeState(int n = 3)
{
  ExperimentConfig config;
  config.n_agents = n;
  return makeSwarmState(config);
}

void scanFraction(SwarmState * state, double fraction)
{
  const int target = static_cast<int>(state->area().cellCount() * fraction);
  int scanned = 0;
  for (const auto & agent : state->agents()) {
    for (const int cell_id : agent.region) {
      if (scanned >= target) {
        return;
      }
      state->markVisited(cell_id);
      ++scanned;
    }
  }
}

}  // namespace

TEST(FailureInjector, KapaliykenHicTetiklenmez)
{
  auto state = makeState();
  FailureInjection config;
  config.enabled = false;
  FailureInjector injector(config, 0);

  scanFraction(&state, 0.9);
  EXPECT_FALSE(injector.update(&state).has_value());
  EXPECT_FALSE(injector.triggered());
  EXPECT_EQ(state.orphanedCells().size(), 0u);
}

TEST(FailureInjector, BelirtilenZamandaTetiklenir)
{
  auto state = makeState();
  FailureInjection config;
  config.enabled = true;
  config.time = 50.0;
  config.agent_id = 1;
  FailureInjector injector(config, 0);

  state.setTime(49.9);
  EXPECT_FALSE(injector.update(&state).has_value());

  state.setTime(50.0);
  const auto failed = injector.update(&state);
  ASSERT_TRUE(failed.has_value());
  EXPECT_EQ(*failed, 1);
  EXPECT_TRUE(injector.triggered());
  EXPECT_EQ(injector.failedAgent(), 1);
  EXPECT_DOUBLE_EQ(injector.failureTime(), 50.0);
}

TEST(FailureInjector, ZamanNegatifseGorevOrtasindaTetiklenir)
{
  // Gorev suresi onceden bilinmedigi icin zaman yerine kapsama ilerlemesi.
  auto state = makeState();
  FailureInjection config;
  config.enabled = true;
  config.time = -1.0;
  config.agent_id = 0;
  FailureInjector injector(config, 0);

  scanFraction(&state, 0.3);
  EXPECT_FALSE(injector.update(&state).has_value());

  scanFraction(&state, 0.55);
  EXPECT_TRUE(injector.update(&state).has_value());
}

TEST(FailureInjector, YalnizcaBirKezTetiklenir)
{
  auto state = makeState();
  FailureInjection config;
  config.enabled = true;
  config.time = 0.0;
  config.agent_id = 2;
  FailureInjector injector(config, 0);

  EXPECT_TRUE(injector.update(&state).has_value());
  for (int i = 0; i < 5; ++i) {
    EXPECT_FALSE(injector.update(&state).has_value());
  }
}

TEST(FailureInjector, RastgeleSecimTohumdanTuretilirVeCanliAjanSecer)
{
  FailureInjection config;
  config.enabled = true;
  config.time = 0.0;
  config.agent_id = -1;

  auto pick = [&config](int seed) {
      auto state = makeState(5);
      FailureInjector injector(config, seed);
      const auto failed = injector.update(&state);
      return failed.value_or(-1);
    };

  EXPECT_EQ(pick(4), pick(4));   // tekrarlanabilir
  std::set<int> picks;
  for (int seed = 0; seed < 25; ++seed) {
    const int id = pick(seed);
    EXPECT_GE(id, 0);
    EXPECT_LT(id, 5);
    picks.insert(id);
  }
  EXPECT_GT(picks.size(), 1u) << "secim tohumdan bagimsiz sabit kalmis";
}

TEST(FailureInjector, ArizaliAjaninTaranmamisAlaniSahipsizHavuzaGecer)
{
  auto state = makeState();
  const int region_size = static_cast<int>(state.agent(1).region.size());
  // Bolgenin ilk yarisini tara.
  for (int i = 0; i < region_size / 2; ++i) {
    state.markVisited(state.agent(1).region[static_cast<std::size_t>(i)]);
  }

  FailureInjection config;
  config.enabled = true;
  config.time = 0.0;
  config.agent_id = 1;
  FailureInjector injector(config, 0);
  ASSERT_TRUE(injector.update(&state).has_value());

  EXPECT_FALSE(state.agent(1).alive);
  EXPECT_TRUE(state.agent(1).region.empty());
  EXPECT_EQ(static_cast<int>(state.orphanedCells().size()), region_size - region_size / 2);
  // Taranmis hucreler havuza girmemeli.
  for (const int cell_id : state.orphanedCells()) {
    EXPECT_FALSE(state.isVisited(cell_id));
  }
}

TEST(FailureInjector, SahipsizHucreVarkenKapsamaTamamlanmaz)
{
  auto state = makeState();
  FailureInjection config;
  config.enabled = true;
  config.time = 0.0;
  config.agent_id = 1;
  FailureInjector injector(config, 0);
  ASSERT_TRUE(injector.update(&state).has_value());

  // Canli ajanlarin tum bolgelerini tara: yine de tamamlanmamali.
  for (const auto & agent : state.agents()) {
    for (const int cell_id : agent.region) {
      state.markVisited(cell_id);
    }
  }
  EXPECT_FALSE(state.coverageComplete());
  EXPECT_GT(state.orphanedCells().size(), 0u);
}

TEST(FailureInjector, DevralmaHucreleriYakinAjanaDagitir)
{
  auto state = makeState();
  FailureInjection config;
  config.enabled = true;
  config.time = 0.0;
  config.agent_id = 1;   // ortadaki serit
  FailureInjector injector(config, 0);
  ASSERT_TRUE(injector.update(&state).has_value());

  const int orphan_count = static_cast<int>(state.orphanedCells().size());
  ASSERT_GT(orphan_count, 0);

  const int distributed = AreaSwapNegotiator::distributeOrphans(&state, 0, 2);
  EXPECT_EQ(distributed, orphan_count);
  EXPECT_TRUE(state.orphanedCells().empty());
  // Iki komsu da pay almali (arizali serit ikisinin arasinda).
  EXPECT_GT(state.agent(0).assignment_changes, 0);
  EXPECT_GT(state.agent(2).assignment_changes, 0);
}

TEST(FailureInjector, TekCanliAjanTumSahipsizAlaniDevralir)
{
  auto state = makeState();
  FailureInjection config;
  config.enabled = true;
  config.time = 0.0;
  config.agent_id = 1;
  FailureInjector injector(config, 0);
  ASSERT_TRUE(injector.update(&state).has_value());

  const int orphan_count = static_cast<int>(state.orphanedCells().size());
  // Arizali ajanla birlikte cagrilirsa hepsi canli olana gitmeli.
  EXPECT_EQ(AreaSwapNegotiator::distributeOrphans(&state, 0, 1), orphan_count);
  EXPECT_TRUE(state.orphanedCells().empty());
}

TEST(FailureInjector, DevralmaSonrasiKapsamaTamamlanabilir)
{
  auto state = makeState();
  FailureInjection config;
  config.enabled = true;
  config.time = 0.0;
  config.agent_id = 1;
  FailureInjector injector(config, 0);
  ASSERT_TRUE(injector.update(&state).has_value());
  AreaSwapNegotiator::distributeOrphans(&state, 0, 2);

  for (const auto & agent : state.agents()) {
    for (const int cell_id : agent.region) {
      state.markVisited(cell_id);
    }
  }
  EXPECT_TRUE(state.coverageComplete());
}

TEST(FailureInjector, DevralmaBuyukDengesizlikYaratir)
{
  // Calismanin asil sorusu icin kritik: ariza, yeniden-atama mekanizmasini
  // gercekten calistiran ana dengesizlik kaynagidir.
  auto state = makeState();
  EXPECT_LT(AreaSwapNegotiator::imbalance(state, 0, 2), 0.05);

  FailureInjection config;
  config.enabled = true;
  config.time = 0.0;
  config.agent_id = 1;
  FailureInjector injector(config, 0);
  ASSERT_TRUE(injector.update(&state).has_value());
  AreaSwapNegotiator::distributeOrphans(&state, 0, 2);

  // Sahipsiz alan tek tarafa daha cok duştuyse dengesizlik esigi asilir.
  EXPECT_GT(state.agent(0).region.size() + state.agent(2).region.size(), 0u);
  EXPECT_TRUE(state.orphanedCells().empty());
}

TEST(FailureInjector, ArizaliAjanDevralamaz)
{
  auto state = makeState();
  state.failAgent(1);
  EXPECT_THROW(state.claimOrphanedCells(1, {0}), std::invalid_argument);
}

TEST(FailureInjector, GenelKapsamaOrani)
{
  auto state = makeState();
  EXPECT_DOUBLE_EQ(swarm_bt_core::globalCoverageRatio(state), 0.0);
  scanFraction(&state, 0.25);
  EXPECT_NEAR(swarm_bt_core::globalCoverageRatio(state), 0.25, 0.01);
}

// --- Bosta kalan ajanin sahipsiz alani ustlenmesi ---

TEST(FailureInjector, MesgulAjanSahipsizAlaniUstlenmez)
{
  auto state = makeState();
  state.failAgent(1);
  ASSERT_GT(state.orphanedCells().size(), 0u);
  // 0 numarali ajanin kendi isi henuz bitmedi.
  EXPECT_EQ(AreaSwapNegotiator::claimOrphansIfIdle(&state, 0), 0);
}

TEST(FailureInjector, BostaKalanAjanSahipsizAlaniUstlenir)
{
  auto state = makeState();
  state.failAgent(1);
  const int orphan_count = static_cast<int>(state.orphanedCells().size());
  ASSERT_GT(orphan_count, 0);

  // 0 numarali ajan kendi bolgesini bitirsin.
  for (const int cell_id : state.agent(0).region) {
    state.markVisited(cell_id);
  }
  EXPECT_EQ(state.remainingCells(0), 0);

  EXPECT_EQ(AreaSwapNegotiator::claimOrphansIfIdle(&state, 0), orphan_count);
  EXPECT_TRUE(state.orphanedCells().empty());
  EXPECT_EQ(state.remainingCells(0), orphan_count);
}

TEST(FailureInjector, ArizaliAjanSahipsizAlaniUstlenmez)
{
  auto state = makeState();
  state.failAgent(1);
  EXPECT_EQ(AreaSwapNegotiator::claimOrphansIfIdle(&state, 1), 0);
}

TEST(FailureInjector, TaranmisSahipsizHucreTekrarUstlenilmez)
{
  auto state = makeState();
  state.failAgent(1);
  for (const int cell_id : state.orphanedCells()) {
    state.markVisited(cell_id);
  }
  for (const int cell_id : state.agent(0).region) {
    state.markVisited(cell_id);
  }
  EXPECT_EQ(AreaSwapNegotiator::claimOrphansIfIdle(&state, 0), 0);
}
