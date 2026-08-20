// Ilgi noktalari: feromon haritasinin anlam kazandigi kaynak (plan Bolum 2.2).
#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/swarm_state.hpp"

using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::SwarmState;
using swarm_bt_core::makeSwarmState;

TEST(InterestPoints, IstenenSayidaNoktaYerlestirilir)
{
  ExperimentConfig config;
  config.sim.interest_points = 12;
  const auto state = makeSwarmState(config, 0);
  EXPECT_EQ(static_cast<int>(state.interestPoints().size()), 12);
}

TEST(InterestPoints, NoktalarBenzersizVeAlanIcinde)
{
  ExperimentConfig config;
  config.sim.interest_points = 40;
  const auto state = makeSwarmState(config, 3);

  const std::set<int> unique(state.interestPoints().begin(), state.interestPoints().end());
  EXPECT_EQ(unique.size(), state.interestPoints().size()) << "tekrar eden nokta var";
  for (const int cell_id : state.interestPoints()) {
    EXPECT_TRUE(state.area().validCell(cell_id));
  }
}

TEST(InterestPoints, TohumTekrarlanabilirFarkliTohumFarkliDagilim)
{
  ExperimentConfig config;
  config.sim.interest_points = 12;
  EXPECT_EQ(makeSwarmState(config, 5).interestPoints(), makeSwarmState(config, 5).interestPoints());
  EXPECT_NE(makeSwarmState(config, 5).interestPoints(), makeSwarmState(config, 6).interestPoints());
}

TEST(InterestPoints, SifirNoktaIstenirseHicYerlestirilmez)
{
  ExperimentConfig config;
  config.sim.interest_points = 0;
  const auto state = makeSwarmState(config, 0);
  EXPECT_TRUE(state.interestPoints().empty());
  EXPECT_FALSE(state.hasInterestPoint(0));
}

TEST(InterestPoints, HucreSayisindanFazlaNoktaIstenemez)
{
  ExperimentConfig config;
  config.sim.interest_points = 10000;
  const auto state = makeSwarmState(config, 0);
  EXPECT_EQ(static_cast<int>(state.interestPoints().size()), state.area().cellCount());
}

TEST(InterestPoints, NoktaSorgusuDogru)
{
  ExperimentConfig config;
  config.sim.interest_points = 12;
  const auto state = makeSwarmState(config, 1);
  const std::set<int> points(state.interestPoints().begin(), state.interestPoints().end());

  for (int cell_id = 0; cell_id < state.area().cellCount(); ++cell_id) {
    EXPECT_EQ(state.hasInterestPoint(cell_id), points.count(cell_id) > 0) << "hucre " << cell_id;
  }
}

TEST(InterestPoints, NegatifNoktaSayisiReddedilir)
{
  ExperimentConfig config;
  config.sim.interest_points = -1;
  EXPECT_THROW(config.validate(), std::invalid_argument);
}

TEST(InterestPoints, IlgiNoktalariAjanKonumlarindanBagimsiz)
{
  // Ayni tohumda kalkis konumlari ile ilgi noktalari ayni akistan uretilse
  // aralarinda yapay bir iliski olusurdu.
  ExperimentConfig config;
  config.sim.interest_points = 12;
  const auto state = makeSwarmState(config, 0);

  int on_agent_cell = 0;
  for (const auto & agent : state.agents()) {
    const int cell_id = state.area().cellAt(agent.position);
    if (cell_id >= 0 && state.hasInterestPoint(cell_id)) {
      ++on_agent_cell;
    }
  }
  EXPECT_LE(on_agent_cell, 1) << "ilgi noktalari ajan konumlariyla ortusuyor";
}
