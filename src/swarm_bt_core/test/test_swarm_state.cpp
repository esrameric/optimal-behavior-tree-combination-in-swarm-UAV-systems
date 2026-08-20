#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "swarm_bt_core/swarm_state.hpp"

using swarm_bt_core::MissionArea;
using swarm_bt_core::SwarmState;

namespace
{
SwarmState makeState(int n)
{
  SwarmState state(MissionArea(400.0, 400.0, 20.0), n, 0.01);
  state.assignEqualStrips();
  return state;
}
}  // namespace

TEST(SwarmState, GecersizAjanSayisiReddedilir)
{
  EXPECT_THROW(SwarmState(MissionArea(400.0, 400.0, 20.0), 0, 0.0), std::invalid_argument);
  auto state = makeState(3);
  EXPECT_THROW(state.agent(3), std::out_of_range);
  EXPECT_THROW(state.agent(-1), std::out_of_range);
}

TEST(SwarmState, EsitSeritBolmesiTumHucreleriTamAmenBirKezKapsar)
{
  for (const int n : {3, 5}) {
    auto state = makeState(n);
    std::set<int> seen;
    int total = 0;
    for (const auto & agent : state.agents()) {
      for (const int cell_id : agent.region) {
        EXPECT_TRUE(seen.insert(cell_id).second) << "hucre " << cell_id << " iki kez atandi";
        ++total;
      }
    }
    EXPECT_EQ(total, state.area().cellCount()) << "N=" << n;
    EXPECT_EQ(static_cast<int>(seen.size()), state.area().cellCount()) << "N=" << n;
  }
}

TEST(SwarmState, SeritlerMumkunOlduguncaEsitBoyutlu)
{
  // 20 sutun / 3 ajan -> 7,7,6 sutun; hucre farki en fazla bir sutun kadar olmali.
  auto state = makeState(3);
  std::size_t min_size = state.agents().front().region.size();
  std::size_t max_size = min_size;
  for (const auto & agent : state.agents()) {
    min_size = std::min(min_size, agent.region.size());
    max_size = std::max(max_size, agent.region.size());
  }
  EXPECT_LE(max_size - min_size, static_cast<std::size_t>(state.area().rows()));
}

TEST(SwarmState, AjanlarKendiSeridininIlkWaypointindeBaslar)
{
  auto state = makeState(5);
  for (const auto & agent : state.agents()) {
    ASSERT_FALSE(agent.region.empty());
    const auto expected = state.area().cellCenter(agent.region.front());
    EXPECT_DOUBLE_EQ(agent.position.x, expected.x);
    EXPECT_DOUBLE_EQ(agent.position.y, expected.y);
  }
}

TEST(SwarmState, KalanAlanOraniZiyaretlerleAzalir)
{
  auto state = makeState(3);
  EXPECT_DOUBLE_EQ(state.remainingRatio(0), 1.0);
  EXPECT_FALSE(state.coverageComplete());

  const auto region = state.agent(0).region;
  for (std::size_t i = 0; i < region.size() / 2; ++i) {
    state.markVisited(region[i]);
  }
  EXPECT_NEAR(state.remainingRatio(0), 0.5, 0.01);

  for (const auto & agent : state.agents()) {
    for (const int cell_id : agent.region) {
      state.markVisited(cell_id);
    }
  }
  EXPECT_TRUE(state.coverageComplete());
  EXPECT_DOUBLE_EQ(state.remainingRatio(0), 0.0);
}

TEST(SwarmState, KapsamaDengesizligiEsitDagilimdaSifir)
{
  auto state = makeState(4);  // 20 sutun / 4 -> tam esit bolunme
  EXPECT_DOUBLE_EQ(state.coverageImbalance(), 0.0);

  // Bir ajanin bolgesini tamamen tara -> dengesizlik artmali.
  for (const int cell_id : state.agent(0).region) {
    state.markVisited(cell_id);
  }
  EXPECT_GT(state.coverageImbalance(), 0.0);
}

TEST(SwarmState, ZamanIlerletilebilir)
{
  auto state = makeState(3);
  EXPECT_DOUBLE_EQ(state.time(), 0.0);
  state.advanceTime(0.1);
  state.advanceTime(0.1);
  EXPECT_NEAR(state.time(), 0.2, 1e-12);
}
