// Plan Bolum 1: mission alani N degisse de BUYUMEZ.
//
// Bu, calismanin karsilastirma dayanagidir: N=5'te ayni alanda daha yogun ucus
// -> daha sik karsilasma -> yeniden-muzakere davranisi strese girer. Alan N ile
// buyuse, olculen fark "olcek etkisi" degil "alan etkisi" olurdu.
#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/swarm_state.hpp"

using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::makeSwarmState;

TEST(MissionAreaInvariant, AlanNDenBagimsiz)
{
  ExperimentConfig config;
  config.n_agents = 3;
  const auto area3 = config.missionArea();

  config.n_agents = 5;
  const auto area5 = config.missionArea();

  EXPECT_DOUBLE_EQ(area3.width(), area5.width());
  EXPECT_DOUBLE_EQ(area3.height(), area5.height());
  EXPECT_DOUBLE_EQ(area3.cellSize(), area5.cellSize());
  EXPECT_EQ(area3.cellCount(), area5.cellCount());
  EXPECT_EQ(area3.cellCount(), 400);
}

TEST(MissionAreaInvariant, HerNDegeriIcinAlanAyni)
{
  ExperimentConfig config;
  int reference = 0;
  for (const int n : {1, 2, 3, 4, 5, 7, 10}) {
    config.n_agents = n;
    const int cells = config.missionArea().cellCount();
    if (reference == 0) {
      reference = cells;
    }
    EXPECT_EQ(cells, reference) << "N=" << n << " alani degistirdi";
  }
}

TEST(MissionAreaInvariant, DroneBasinaAlanNIleTersOrantiliAzalir)
{
  ExperimentConfig config;
  config.n_agents = 3;
  const double per_agent_3 = config.cellsPerAgent();
  config.n_agents = 5;
  const double per_agent_5 = config.cellsPerAgent();

  EXPECT_NEAR(per_agent_3, 400.0 / 3.0, 1e-9);
  EXPECT_NEAR(per_agent_5, 400.0 / 5.0, 1e-9);
  EXPECT_LT(per_agent_5, per_agent_3);
  // Alan sabit oldugu icin oran tam olarak N oranidir.
  EXPECT_NEAR(per_agent_3 / per_agent_5, 5.0 / 3.0, 1e-9);
}

TEST(MissionAreaInvariant, SuruDurumuNDenBagimsizAyniHucreKumesiniKapsar)
{
  std::vector<int> cell_counts;
  for (const int n : {3, 5}) {
    ExperimentConfig config;
    config.n_agents = n;
    const auto state = makeSwarmState(config);

    std::set<int> covered;
    for (const auto & agent : state.agents()) {
      covered.insert(agent.region.begin(), agent.region.end());
    }
    EXPECT_EQ(static_cast<int>(covered.size()), state.area().cellCount()) << "N=" << n;
    cell_counts.push_back(state.area().cellCount());
  }
  EXPECT_EQ(cell_counts[0], cell_counts[1]);
}

TEST(MissionAreaInvariant, AlanDegisimiYalnizcaConfigUzerindenOlur)
{
  // Alani buyutmek isteyen (Bolum 9 opsiyonel kontrol deneyi) bunu yalnizca
  // sim.area_side uzerinden yapabilmeli; N'i degistirmek yeterli olmamali.
  ExperimentConfig config;
  config.n_agents = 5;
  config.sim.area_side = 600.0;
  EXPECT_EQ(config.missionArea().cols(), 30);
  EXPECT_EQ(config.missionArea().cellCount(), 900);
}
