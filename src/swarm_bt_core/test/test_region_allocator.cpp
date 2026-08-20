// P3 - baslangic alan atama algoritmalari (plan Bolum 2.1 adim 2, Bolum 3).
#include <gtest/gtest.h>

#include <map>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include "swarm_bt_core/area_swap.hpp"
#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/region_allocator.hpp"
#include "swarm_bt_core/swarm_state.hpp"

using swarm_bt_core::AllocationAlgorithm;
using swarm_bt_core::AreaSwapNegotiator;
using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::SwarmState;
using swarm_bt_core::allocateRegions;
using swarm_bt_core::makeSwarmState;

namespace
{

SwarmState makeState(AllocationAlgorithm algorithm, int n = 3)
{
  ExperimentConfig config;
  config.n_agents = n;
  config.p3 = algorithm;
  return makeSwarmState(config);
}

std::vector<AllocationAlgorithm> allAlgorithms()
{
  return {
    AllocationAlgorithm::kStaticEqual,
    AllocationAlgorithm::kContractNet,
    AllocationAlgorithm::kCbba};
}

std::string nameOf(AllocationAlgorithm algorithm)
{
  return "P3" + swarm_bt_core::toLetter(algorithm);
}

}  // namespace

TEST(RegionAllocator, SutunHucreleri)
{
  const swarm_bt_core::MissionArea area(400.0, 400.0, 20.0);
  const auto cells = swarm_bt_core::columnCells(area, 3);
  EXPECT_EQ(static_cast<int>(cells.size()), area.rows());
  for (const int cell_id : cells) {
    EXPECT_EQ(area.colOf(cell_id), 3);
  }
  EXPECT_THROW(swarm_bt_core::columnCells(area, -1), std::out_of_range);
  EXPECT_THROW(swarm_bt_core::columnCells(area, area.cols()), std::out_of_range);
}

TEST(RegionAllocator, HerAlgoritmaTumAlaniTamAmenBirKezKapsar)
{
  for (const auto algorithm : allAlgorithms()) {
    for (const int n : {3, 5}) {
      auto state = makeState(algorithm, n);
      std::set<int> covered;
      int total = 0;
      for (const auto & agent : state.agents()) {
        for (const int cell_id : agent.region) {
          EXPECT_TRUE(covered.insert(cell_id).second)
            << nameOf(algorithm) << " N=" << n << ": hucre " << cell_id << " iki kez atandi";
          ++total;
        }
      }
      EXPECT_EQ(total, state.area().cellCount()) << nameOf(algorithm) << " N=" << n;
    }
  }
}

TEST(RegionAllocator, HerAjanEnAzBirSutunAlir)
{
  for (const auto algorithm : allAlgorithms()) {
    for (const int n : {3, 5}) {
      auto state = makeState(algorithm, n);
      for (const auto & agent : state.agents()) {
        EXPECT_FALSE(agent.region.empty())
          << nameOf(algorithm) << " N=" << n << ": ajan " << agent.id << " bos kaldi";
      }
    }
  }
}

TEST(RegionAllocator, BolgelerTamSutunlardanOlusur)
{
  // Hucre yerine sutun atanmasinin sebebi bicerdover taramanin dogal birimi
  // olmasi: yarim sutun, ajani ayni sutuna iki kez ugramaya zorlardi.
  for (const auto algorithm : allAlgorithms()) {
    auto state = makeState(algorithm, 3);
    for (const auto & agent : state.agents()) {
      std::map<int, int> cells_per_column;
      for (const int cell_id : agent.region) {
        ++cells_per_column[state.area().colOf(cell_id)];
      }
      for (const auto & entry : cells_per_column) {
        EXPECT_EQ(entry.second, state.area().rows())
          << nameOf(algorithm) << ": sutun " << entry.first << " yarim atanmis";
      }
    }
  }
}

TEST(RegionAllocator, KapasiteTavaniAsilmaz)
{
  // Tavansiz bir ihalede bitisik sutunlarin marjinal maliyeti dusuk kaldigi
  // icin tek ajan tum alani kazanabilirdi.
  for (const auto algorithm : allAlgorithms()) {
    for (const int n : {3, 5}) {
      auto state = makeState(algorithm, n);
      const int columns = state.area().cols();
      const int capacity = (columns + n - 1) / n;
      for (const auto & agent : state.agents()) {
        const int agent_columns = static_cast<int>(agent.region.size()) / state.area().rows();
        EXPECT_LE(agent_columns, capacity)
          << nameOf(algorithm) << " N=" << n << ": ajan " << agent.id << " tavani asti";
      }
    }
  }
}

TEST(RegionAllocator, BaslangicKonumlariAlgoritmadanBagimsiz)
{
  // Olculen fark atama algoritmasinin olmali, kalkis geometrisinin degil.
  const auto reference = makeState(AllocationAlgorithm::kStaticEqual, 5);
  for (const auto algorithm : allAlgorithms()) {
    auto state = makeState(algorithm, 5);
    for (int id = 0; id < 5; ++id) {
      EXPECT_DOUBLE_EQ(state.agent(id).position.x, reference.agent(id).position.x)
        << nameOf(algorithm) << " ajan " << id;
      EXPECT_DOUBLE_EQ(state.agent(id).position.y, reference.agent(id).position.y)
        << nameOf(algorithm) << " ajan " << id;
    }
  }
}

TEST(RegionAllocator, IhaleliAlgoritmalarStatiktenFarkliDagilimUretir)
{
  auto regionSignature = [](const SwarmState & state) {
      std::vector<std::size_t> sizes;
      for (const auto & agent : state.agents()) {
        sizes.push_back(agent.region.size());
      }
      return sizes;
    };

  const auto st = makeState(AllocationAlgorithm::kStaticEqual, 3);
  const auto cn = makeState(AllocationAlgorithm::kContractNet, 3);
  const auto cbba = makeState(AllocationAlgorithm::kCbba, 3);

  // En az bir ihaleli algoritma statikten farkli bir sonuc vermeli; aksi halde
  // P3 ekseni OFAT taramasinda hicbir sey olcmezdi.
  const bool differs =
    (regionSignature(cn) != regionSignature(st)) ||
    (regionSignature(cbba) != regionSignature(st)) ||
    (cn.agent(0).region != st.agent(0).region) ||
    (cbba.agent(0).region != st.agent(0).region);
  EXPECT_TRUE(differs) << "P3 secenekleri ayni dagilimi uretiyor";
}

TEST(RegionAllocator, TahsisTekrarlanabilir)
{
  for (const auto algorithm : allAlgorithms()) {
    const auto first = makeState(algorithm, 5);
    const auto second = makeState(algorithm, 5);
    for (int id = 0; id < 5; ++id) {
      EXPECT_EQ(first.agent(id).region, second.agent(id).region) << nameOf(algorithm);
    }
  }
}

TEST(RegionAllocator, MarjinalMaliyetPozitif)
{
  auto state = makeState(AllocationAlgorithm::kStaticEqual, 3);
  // Ajanin kendi bolgesinde OLMAYAN bir sutunu eklemek turu uzatmali.
  const int foreign_column = state.area().colOf(state.agent(2).region.front());
  EXPECT_GT(swarm_bt_core::marginalColumnCost(state, 0, foreign_column), 0.0);
}

TEST(RegionAllocator, BosDurumReddedilir)
{
  EXPECT_THROW(allocateRegions(nullptr, AllocationAlgorithm::kCbba), std::invalid_argument);
}
