// Plan Bolum 6: atama kararliligi ve churn orani icin otomatik olay kaydi.
#include <gtest/gtest.h>

#include <vector>

#include "swarm_bt_core/area_swap.hpp"
#include "swarm_bt_core/assignment_log.hpp"
#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/failure_injector.hpp"
#include "swarm_bt_core/swarm_state.hpp"

using swarm_bt_core::AreaSwapNegotiator;
using swarm_bt_core::AssignmentChangeEvent;
using swarm_bt_core::AssignmentLog;
using swarm_bt_core::AssignmentReason;
using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::SwarmState;
using swarm_bt_core::makeSwarmState;

namespace
{

SwarmState makeState(int n = 3)
{
  ExperimentConfig config;
  config.n_agents = n;
  config.sim.interest_points = 0;
  return makeSwarmState(config, 0);
}

void scanFraction(SwarmState * state, int agent_id, double fraction)
{
  const auto region = state->agent(agent_id).region;
  const auto count = static_cast<std::size_t>(region.size() * fraction);
  for (std::size_t i = 0; i < count; ++i) {
    state->markVisitedBy(agent_id, region[i]);
  }
}

}  // namespace

TEST(AssignmentLog, BosKayitOlaySizBaslar)
{
  const AssignmentLog log;
  EXPECT_TRUE(log.empty());
  EXPECT_EQ(log.size(), 0u);
  EXPECT_EQ(log.countByReason(AssignmentReason::kAreaSwap), 0);
}

TEST(AssignmentLog, SebepBazindaSayim)
{
  AssignmentLog log;
  log.record(AssignmentChangeEvent{0.0, 0, AssignmentReason::kAreaSwap, 1, 5, 10, 8, 1});
  log.record(AssignmentChangeEvent{1.0, 1, AssignmentReason::kAreaSwap, 0, 5, 12, 9, 1});
  log.record(AssignmentChangeEvent{2.0, 2, AssignmentReason::kFailure, -1, 3, 0, 0, 1});

  EXPECT_EQ(log.countByReason(AssignmentReason::kAreaSwap), 2);
  EXPECT_EQ(log.countByReason(AssignmentReason::kFailure), 1);
  EXPECT_EQ(log.countByReason(AssignmentReason::kJointScan), 0);
}

TEST(AssignmentLog, AjanBasinaDegisiklikOrtalamasi)
{
  AssignmentLog log;
  for (int i = 0; i < 6; ++i) {
    log.record(AssignmentChangeEvent{});
  }
  EXPECT_DOUBLE_EQ(log.changesPerAgent(3), 2.0);
  EXPECT_THROW(log.changesPerAgent(0), std::invalid_argument);
}

TEST(AssignmentLog, BosaltmaYalnizcaYeniOlaylariVerir)
{
  // Faz 2'de ROS2 dugumu her tick bunu cagirip topic'e basar; ayni olay iki
  // kez yayinlanmamali.
  AssignmentLog log;
  log.record(AssignmentChangeEvent{});
  log.record(AssignmentChangeEvent{});
  EXPECT_EQ(log.drain().size(), 2u);
  EXPECT_TRUE(log.drain().empty());

  log.record(AssignmentChangeEvent{});
  EXPECT_EQ(log.drain().size(), 1u);
  EXPECT_EQ(log.size(), 3u) << "bosaltma kaydi silmemeli";
}

TEST(AssignmentLog, SifirlamaHerSeyiTemizler)
{
  AssignmentLog log;
  log.record(AssignmentChangeEvent{});
  log.drain();
  log.clear();
  EXPECT_TRUE(log.empty());
  EXPECT_TRUE(log.drain().empty());
}

// --- SwarmState entegrasyonu ---

TEST(AssignmentLog, SayacVeKayitBirlikteGuncellenir)
{
  // Atama degisikliginin TEK gecis noktasi olmasinin amaci: sayac ile kayit
  // birbirinden sapamasin.
  auto state = makeState();
  EXPECT_EQ(state.agent(0).assignment_changes, 0);
  EXPECT_TRUE(state.assignmentLog().empty());

  state.recordAssignmentChange(0, AssignmentReason::kAreaSwap, 1, 7);

  EXPECT_EQ(state.agent(0).assignment_changes, 1);
  ASSERT_EQ(state.assignmentLog().size(), 1u);
  const auto & event = state.assignmentLog().events().front();
  EXPECT_EQ(event.agent_id, 0);
  EXPECT_EQ(event.peer_id, 1);
  EXPECT_EQ(event.cells_changed, 7);
  EXPECT_EQ(event.change_index, 1);
  EXPECT_EQ(event.reason, AssignmentReason::kAreaSwap);
}

TEST(AssignmentLog, ArizaOlayiKaydedilir)
{
  auto state = makeState();
  state.failAgent(1);

  ASSERT_EQ(state.assignmentLog().size(), 1u);
  const auto & event = state.assignmentLog().events().front();
  EXPECT_EQ(event.agent_id, 1);
  EXPECT_EQ(event.reason, AssignmentReason::kFailure);
  EXPECT_GT(event.cells_changed, 0) << "sahipsiz kalan hucre sayisi kaydedilmeli";
  EXPECT_EQ(event.region_cells, 0);
}

TEST(AssignmentLog, DevralmaOlayiKaydedilir)
{
  auto state = makeState();
  state.failAgent(1);
  const auto orphans = state.orphanedCells();
  ASSERT_FALSE(orphans.empty());

  state.claimOrphanedCells(0, orphans);

  const auto takeovers =
    state.assignmentLog().countByReason(AssignmentReason::kOrphanTakeover);
  EXPECT_EQ(takeovers, 1);
  const auto & event = state.assignmentLog().events().back();
  EXPECT_EQ(event.agent_id, 0);
  EXPECT_EQ(event.cells_changed, static_cast<int>(orphans.size()));
}

TEST(AssignmentLog, TakasOlayiHerIkiTarafaDaKaydedilir)
{
  auto state = makeState();
  scanFraction(&state, 0, 0.85);

  const AreaSwapNegotiator negotiator(0.10);
  const auto proposal = negotiator.buildProposal(state, 0, 1);
  ASSERT_TRUE(proposal.has_value());
  negotiator.apply(&state, *proposal);

  EXPECT_EQ(state.assignmentLog().countByReason(AssignmentReason::kAreaSwap), 2);
  const auto & events = state.assignmentLog().events();
  EXPECT_NE(events[events.size() - 2].agent_id, events.back().agent_id);
  EXPECT_EQ(events.back().peer_id, events[events.size() - 2].agent_id);
}

TEST(AssignmentLog, OrtakTaramaOlayiKaydedilir)
{
  auto state = makeState();
  const auto boundary = AreaSwapNegotiator::boundaryCells(state, 0, 1);
  ASSERT_FALSE(boundary.empty());

  AreaSwapNegotiator::startJointScan(&state, 0, 1);
  EXPECT_EQ(state.assignmentLog().countByReason(AssignmentReason::kJointScan), 2);
}

TEST(AssignmentLog, AtamaKararliligiKayittanTuretilebilir)
{
  // Bolum 6 metrigi ile olay kaydi ayni kaynaktan gelmeli.
  auto state = makeState();
  state.recordAssignmentChange(0, AssignmentReason::kAreaSwap, 1, 3);
  state.recordAssignmentChange(1, AssignmentReason::kAreaSwap, 0, 3);
  state.recordAssignmentChange(0, AssignmentReason::kJointScan, 2, 4);

  double from_counters = 0.0;
  for (const auto & agent : state.agents()) {
    from_counters += agent.assignment_changes;
  }
  EXPECT_DOUBLE_EQ(
    state.assignmentLog().changesPerAgent(state.agentCount()),
    from_counters / state.agentCount());
}

TEST(AssignmentLog, OlaylarZamanDamgasiTasir)
{
  auto state = makeState();
  state.setTime(12.5);
  state.recordAssignmentChange(0, AssignmentReason::kAreaSwap, 1, 2);
  EXPECT_DOUBLE_EQ(state.assignmentLog().events().front().time, 12.5);
}
