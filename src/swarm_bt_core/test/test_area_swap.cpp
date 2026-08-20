// Plan Bolum 2.2: alan takasi tetikleme kosulu ve kabul/red mantigi.
#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "swarm_bt_core/area_swap.hpp"
#include "swarm_bt_core/experiment_config.hpp"

using swarm_bt_core::AreaSwapNegotiator;
using swarm_bt_core::ExperimentConfig;
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

/// Ajanin bolgesinin ilk `count` hucresini taranmis isaretler.
void markScanned(SwarmState * state, int agent_id, int count)
{
  const auto region = state->agent(agent_id).region;
  for (int i = 0; i < count && i < static_cast<int>(region.size()); ++i) {
    state->markVisited(region[static_cast<std::size_t>(i)]);
  }
}

}  // namespace

TEST(AreaSwap, GecersizEsikReddedilir)
{
  EXPECT_THROW(AreaSwapNegotiator(-0.1), std::invalid_argument);
  EXPECT_THROW(AreaSwapNegotiator(1.5), std::invalid_argument);
  EXPECT_NO_THROW(AreaSwapNegotiator(0.0));
  EXPECT_NO_THROW(AreaSwapNegotiator(1.0));
}

TEST(AreaSwap, DengeliDurumdaTeklifYok)
{
  auto state = makeState();
  const AreaSwapNegotiator negotiator(0.30);

  EXPECT_DOUBLE_EQ(AreaSwapNegotiator::imbalance(state, 0, 1), 0.0);
  EXPECT_FALSE(negotiator.imbalanceAboveThreshold(state, 0, 1));
  EXPECT_FALSE(negotiator.buildProposal(state, 0, 1).has_value());
}

TEST(AreaSwap, EsikAsilincaTeklifDogar)
{
  auto state = makeState();
  // 0 numarali ajan bolgesinin yarisini tarasin -> kalan orani ~0.5.
  markScanned(&state, 0, static_cast<int>(state.agent(0).region.size() / 2));

  const AreaSwapNegotiator negotiator(0.30);
  EXPECT_GT(AreaSwapNegotiator::imbalance(state, 0, 1), 0.30);
  EXPECT_TRUE(negotiator.imbalanceAboveThreshold(state, 0, 1));

  const auto proposal = negotiator.buildProposal(state, 0, 1);
  ASSERT_TRUE(proposal.has_value());
  // Kalan alani FAZLA olan (1 numarali ajan) teklif etmeli.
  EXPECT_EQ(proposal->proposer_id, 1);
  EXPECT_EQ(proposal->receiver_id, 0);
  EXPECT_FALSE(proposal->offered_cells.empty());
}

TEST(AreaSwap, EsikTamSinirdaTetiklenmez)
{
  // Kosul kesin buyuk: kalan_alan_farki > esik_degeri
  auto state = makeState();
  const AreaSwapNegotiator strict(1.0);
  markScanned(&state, 0, static_cast<int>(state.agent(0).region.size()));
  EXPECT_DOUBLE_EQ(AreaSwapNegotiator::imbalance(state, 0, 1), 1.0);
  EXPECT_FALSE(strict.imbalanceAboveThreshold(state, 0, 1));
}

TEST(AreaSwap, TurMesafesiKalanHucrelerUzerindenHesaplanir)
{
  auto state = makeState();
  const double full = AreaSwapNegotiator::tourLength(state, 0);
  EXPECT_GT(full, 0.0);

  markScanned(&state, 0, static_cast<int>(state.agent(0).region.size()));
  EXPECT_DOUBLE_EQ(AreaSwapNegotiator::tourLength(state, 0), 0.0);
}

TEST(AreaSwap, HucreEklemekTuruUzatirCikarmakKisaltir)
{
  auto state = makeState();
  const double base = AreaSwapNegotiator::tourLength(state, 0);
  const std::vector<int> extra = {state.agent(1).region.front()};

  EXPECT_GT(AreaSwapNegotiator::tourLengthWith(state, 0, extra), base);

  const std::vector<int> own = {state.agent(0).region.back()};
  EXPECT_LT(AreaSwapNegotiator::tourLengthWithout(state, 0, own), base);
}

TEST(AreaSwap, ZatenTaranmisHucreTuruEtkilemez)
{
  auto state = makeState();
  const int cell = state.agent(1).region.front();
  state.markVisited(cell);
  const double base = AreaSwapNegotiator::tourLength(state, 0);
  EXPECT_DOUBLE_EQ(AreaSwapNegotiator::tourLengthWith(state, 0, {cell}), base);
}

TEST(AreaSwap, TeklifEdilenHucreleAliciyaYakinSecilir)
{
  auto state = makeState();
  markScanned(&state, 0, static_cast<int>(state.agent(0).region.size() * 0.8));

  const AreaSwapNegotiator negotiator(0.30);
  const auto proposal = negotiator.buildProposal(state, 0, 1);
  ASSERT_TRUE(proposal.has_value());

  // Teklif edilen hucrelerin aliciya ortalama uzakligi, teklif edenin
  // devretmedigi hucrelerin uzakligindan kucuk olmali.
  const auto & area = state.area();
  const auto receiver_position = state.agent(proposal->receiver_id).position;
  double offered_sum = 0.0;
  for (const int cell : proposal->offered_cells) {
    offered_sum += swarm_bt_core::distance(receiver_position, area.cellCenter(cell));
  }
  const double offered_mean = offered_sum / proposal->offered_cells.size();

  const std::set<int> offered(proposal->offered_cells.begin(), proposal->offered_cells.end());
  double kept_sum = 0.0;
  int kept_count = 0;
  for (const int cell : state.remainingCellIds(proposal->proposer_id)) {
    if (offered.count(cell) == 0) {
      kept_sum += swarm_bt_core::distance(receiver_position, area.cellCenter(cell));
      ++kept_count;
    }
  }
  ASSERT_GT(kept_count, 0);
  EXPECT_LT(offered_mean, kept_sum / kept_count);
}

TEST(AreaSwap, FaydaOlcutuToplamMesafeUzerinden)
{
  auto state = makeState();
  markScanned(&state, 0, static_cast<int>(state.agent(0).region.size() * 0.9));

  const AreaSwapNegotiator negotiator(0.30);
  const auto proposal = negotiator.buildProposal(state, 0, 1);
  ASSERT_TRUE(proposal.has_value());

  // Alici her zaman is ustlenir -> kendi mesafesi asla azalmaz.
  EXPECT_GE(proposal->receiver_cost, 0.0);
  // Teklif eden her zaman is birakir -> kazanci negatif olmamali.
  EXPECT_GE(proposal->proposer_gain, 0.0);
  // Karar olcutu ikisinin karsilastirmasi.
  EXPECT_EQ(
    proposal->reducesTotalDistance(), proposal->receiver_cost < proposal->proposer_gain);
}

TEST(AreaSwap, UygulamaHucreleriDevredipDengesizligiAzaltir)
{
  auto state = makeState();
  markScanned(&state, 0, static_cast<int>(state.agent(0).region.size() * 0.8));

  const AreaSwapNegotiator negotiator(0.30);
  const auto proposal = negotiator.buildProposal(state, 0, 1);
  ASSERT_TRUE(proposal.has_value());

  const double before = AreaSwapNegotiator::imbalance(state, 0, 1);
  const int proposer_before = state.remainingCells(proposal->proposer_id);
  const int receiver_before = state.remainingCells(proposal->receiver_id);
  const int transferred = static_cast<int>(proposal->offered_cells.size());

  negotiator.apply(&state, *proposal);

  EXPECT_EQ(state.remainingCells(proposal->proposer_id), proposer_before - transferred);
  EXPECT_EQ(state.remainingCells(proposal->receiver_id), receiver_before + transferred);
  EXPECT_LT(AreaSwapNegotiator::imbalance(state, 0, 1), before);
}

TEST(AreaSwap, UygulamaHicbirHucreyiKaybetmezVeCogaltmaz)
{
  auto state = makeState(5);
  markScanned(&state, 2, static_cast<int>(state.agent(2).region.size() * 0.7));

  auto allCells = [](const SwarmState & s) {
      std::vector<int> cells;
      for (const auto & agent : s.agents()) {
        cells.insert(cells.end(), agent.region.begin(), agent.region.end());
      }
      std::sort(cells.begin(), cells.end());
      return cells;
    };
  const auto before = allCells(state);

  const AreaSwapNegotiator negotiator(0.20);
  const auto proposal = negotiator.buildProposal(state, 2, 3);
  ASSERT_TRUE(proposal.has_value());
  negotiator.apply(&state, *proposal);

  const auto after = allCells(state);
  EXPECT_EQ(before, after) << "takas hucre kaybetti ya da cogaltti";
  EXPECT_EQ(std::set<int>(after.begin(), after.end()).size(), after.size())
    << "bir hucre iki ajana birden atanmis";
}

TEST(AreaSwap, UygulamaAtamaDegisiklikSayaclariniArtirir)
{
  // Bolum 6 metrigi: atama kararliligi = atanmis alanin kac kez degistigi.
  auto state = makeState();
  markScanned(&state, 0, static_cast<int>(state.agent(0).region.size() * 0.8));

  const AreaSwapNegotiator negotiator(0.30);
  const auto proposal = negotiator.buildProposal(state, 0, 1);
  ASSERT_TRUE(proposal.has_value());

  EXPECT_EQ(state.agent(0).assignment_changes, 0);
  EXPECT_EQ(state.agent(1).assignment_changes, 0);
  negotiator.apply(&state, *proposal);
  EXPECT_EQ(state.agent(0).assignment_changes, 1);
  EXPECT_EQ(state.agent(1).assignment_changes, 1);
}

TEST(AreaSwap, UygulamaSonrasiRotaYenidenPlanlanir)
{
  auto state = makeState();
  markScanned(&state, 0, static_cast<int>(state.agent(0).region.size() * 0.8));
  state.agent(0).next_waypoint = 42;

  const AreaSwapNegotiator negotiator(0.30);
  const auto proposal = negotiator.buildProposal(state, 0, 1);
  ASSERT_TRUE(proposal.has_value());
  negotiator.apply(&state, *proposal);

  EXPECT_EQ(state.agent(0).next_waypoint, 0u);
  EXPECT_EQ(state.agent(1).next_waypoint, 0u);
}

TEST(AreaSwap, AjanKendisiyleTakasYapamaz)
{
  auto state = makeState();
  const AreaSwapNegotiator negotiator(0.30);
  EXPECT_THROW(negotiator.buildProposal(state, 1, 1), std::invalid_argument);
}

TEST(AreaSwap, BosTeklifUygulamakDurumuDegistirmez)
{
  auto state = makeState();
  const AreaSwapNegotiator negotiator(0.30);
  swarm_bt_core::SwapProposal empty;
  empty.proposer_id = 0;
  empty.receiver_id = 1;
  const auto before = state.agent(0).region;
  negotiator.apply(&state, empty);
  EXPECT_EQ(state.agent(0).region, before);
  EXPECT_EQ(state.agent(0).assignment_changes, 0);
}
