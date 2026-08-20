// Negotiation alt-agacinin BAGIMSIZ birim testleri (plan Bolum 5/Faz 0).
//
// Alt-agac tek basina, ana agactan ve drone SAYISINDAN bagimsiz olarak
// yuklenip tiklenir. Testler plan Bolum 2.2'deki uc dalin her birini ve
// dallar arasi oncelik sirasini dogrular.
#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>

#include <memory>
#include <string>

#include "swarm_bt_core/bt_context.hpp"
#include "swarm_bt_core/bt_nodes.hpp"
#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/swarm_state.hpp"

#ifndef SWARM_BT_XML_DIR
#error "SWARM_BT_XML_DIR tanimli degil (CMakeLists.txt'e bakin)"
#endif

using swarm_bt_core::AreaSwapNegotiator;
using swarm_bt_core::BtContext;
using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::SwarmState;
using swarm_bt_core::makeSwarmState;

namespace
{

/// Alt-agaci tek basina calistirilabilir hale getiren asgari kosum takimi.
class NegotiationHarness
{
public:
  explicit NegotiationHarness(const ExperimentConfig & config, int seed = 0)
  : config_(config),
    state_(makeSwarmState(config, seed)),
    negotiator_(config.swap_threshold)
  {
    context_.state = &state_;
    context_.config = &config_;
    context_.negotiator = &negotiator_;

    swarm_bt_core::registerSwarmNodes(&factory_, &context_);
    factory_.registerBehaviorTreeFromFile(
      std::string(SWARM_BT_XML_DIR) + "/negotiation_subtree.xml");
  }

  /// Alt-agaci verilen ajan icin bir kez tikler.
  BT::NodeStatus tickFor(int agent_id)
  {
    auto blackboard = BT::Blackboard::create();
    blackboard->set(swarm_bt_core::bt_keys::kAgentId, agent_id);
    blackboard->set(swarm_bt_core::bt_keys::kPeerId, -1);
    auto tree = factory_.createTree("NegotiationSubtree", blackboard);
    return tree.tickOnce();
  }

  SwarmState & state() {return state_;}
  BtContext & context() {return context_;}
  const swarm_bt_core::BtCounters & counters() const {return context_.counters;}

private:
  ExperimentConfig config_;
  SwarmState state_;
  AreaSwapNegotiator negotiator_;
  BtContext context_;
  BT::BehaviorTreeFactory factory_;
};

ExperimentConfig baseConfig(int n_agents = 3)
{
  ExperimentConfig config;
  config.n_agents = n_agents;
  config.sim.interest_points = 0;   // feromon dallarini varsayilan olarak kapali tut
  return config;
}

/// Ajanin bolgesinin verilen oranini taranmis isaretler.
void scanFraction(SwarmState * state, int agent_id, double fraction)
{
  const auto region = state->agent(agent_id).region;
  const auto count = static_cast<std::size_t>(region.size() * fraction);
  for (std::size_t i = 0; i < count; ++i) {
    state->markVisitedBy(agent_id, region[i]);
  }
}

}  // namespace

TEST(NegotiationSubtree, TekBasinaYuklenipTicklenebilir)
{
  // Plan: alt-agac drone sayisindan bagimsiz, tek basina dogrulanabilir olmali.
  NegotiationHarness harness(baseConfig());
  EXPECT_NO_THROW(harness.tickFor(0));
}

TEST(NegotiationSubtree, BekleyenKarsilasmaYoksaBasarisiz)
{
  NegotiationHarness harness(baseConfig());
  EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::FAILURE);
  EXPECT_EQ(harness.counters().status_exchanges, 0);
}

TEST(NegotiationSubtree, HicbirEsikAsilmazsaBilgiPaylasimiDaliCalisir)
{
  // Plan Bolum 2.2 ucuncu dal: sadece_bilgi_paylas_ve_devam_et()
  NegotiationHarness harness(baseConfig());
  harness.context().queueEncounter(0, 1);

  EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(harness.counters().status_exchanges, 1);
  EXPECT_EQ(harness.counters().info_shares, 1);
  EXPECT_EQ(harness.counters().swaps_applied, 0);
  EXPECT_EQ(harness.counters().joint_scans, 0);
}

TEST(NegotiationSubtree, DurumBilgisiDegisimiOrtagiKuyruktanAlir)
{
  // Bir karsilasma tam olarak bir muzakere uretmeli.
  NegotiationHarness harness(baseConfig());
  harness.context().queueEncounter(0, 1);

  EXPECT_TRUE(harness.context().hasPendingEncounter(0));
  harness.tickFor(0);
  EXPECT_FALSE(harness.context().hasPendingEncounter(0));

  // Ikinci tick'te bekleyen karsilasma yok -> basarisiz.
  EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::FAILURE);
  EXPECT_EQ(harness.counters().status_exchanges, 1);
}

TEST(NegotiationSubtree, DogrudanMesajKapaliysaMuzakereYurumez)
{
  // P5a kapali: ajanlar birbirinin durumunu ogrenemez.
  auto config = baseConfig();
  config.p5 = swarm_bt_core::CommunicationMechanisms::fromLetters("bc");
  NegotiationHarness harness(config);
  harness.context().queueEncounter(0, 1);

  EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::FAILURE);
  EXPECT_EQ(harness.counters().status_exchanges, 1);   // ortak yine de kuyruktan alindi
  EXPECT_EQ(harness.counters().info_shares, 0);
  EXPECT_EQ(harness.counters().swaps_applied, 0);
}

TEST(NegotiationSubtree, DengesizlikEsigiAsilmazsaTakasDaliDenenmez)
{
  NegotiationHarness harness(baseConfig());
  harness.context().queueEncounter(0, 1);
  harness.tickFor(0);
  EXPECT_EQ(harness.counters().swap_proposals, 0);
}

TEST(NegotiationSubtree, DengesizlikVarkenTakasTeklifiKurulur)
{
  // Plan Bolum 2.2 birinci dal: kalan_alan_farki > esik_degeri -> TAKAS
  NegotiationHarness harness(baseConfig());
  scanFraction(&harness.state(), 0, 0.8);   // 0 numarali ajan cok ilerledi
  harness.context().queueEncounter(0, 1);

  const auto status = harness.tickFor(0);
  EXPECT_GT(harness.counters().swap_proposals, 0);
  // Teklif kurulmali; kabul edilip edilmemesi fayda hesabina bagli.
  if (harness.counters().swaps_applied > 0) {
    EXPECT_EQ(status, BT::NodeStatus::SUCCESS);
  } else {
    // Reddedilirse alt dallara duşulmeli, agac yine sonuca baglanmali.
    EXPECT_EQ(status, BT::NodeStatus::SUCCESS);
    EXPECT_EQ(harness.counters().info_shares, 1);
  }
}

TEST(NegotiationSubtree, TakasReddedilirseBilgiPaylasimiDalinaDuşulur)
{
  // Fallback sirasinin amaci: her karsilasma bir sonuca baglanmali.
  NegotiationHarness harness(baseConfig());
  scanFraction(&harness.state(), 0, 0.95);
  harness.context().queueEncounter(0, 1);

  EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(
    harness.counters().swaps_applied + harness.counters().joint_scans +
    harness.counters().info_shares, 1)
    << "tam olarak bir dal sonuca baglanmali";
}

TEST(NegotiationSubtree, SinirdaFeromonYuksekseOrtakTaramaBaslar)
{
  // Plan Bolum 2.2 ikinci dal: sinir_bolgesinde_feromon_yuksek() -> BIRLIKTE TARA
  auto config = baseConfig();
  config.sim.joint_scan_threshold = 0.01;
  NegotiationHarness harness(config);

  // Iki ajanin sinirindaki hucrelere feromon birak.
  const auto boundary = AreaSwapNegotiator::boundaryCells(harness.state(), 0, 1);
  ASSERT_FALSE(boundary.empty()) << "bitisik ajanlarin sinir hucresi yok";
  for (const int cell_id : boundary) {
    harness.state().interest().deposit(cell_id, 1.0);
  }

  harness.context().queueEncounter(0, 1);
  EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::SUCCESS);
  EXPECT_EQ(harness.counters().joint_scans, 1);
  EXPECT_EQ(harness.counters().info_shares, 0);
}

TEST(NegotiationSubtree, OrtakTaramaHucreleriHerIkiAjanaDaEklenir)
{
  auto config = baseConfig();
  config.sim.joint_scan_threshold = 0.01;
  NegotiationHarness harness(config);

  const auto boundary = AreaSwapNegotiator::boundaryCells(harness.state(), 0, 1);
  ASSERT_FALSE(boundary.empty());
  for (const int cell_id : boundary) {
    harness.state().interest().deposit(cell_id, 1.0);
  }

  const auto size_before_0 = harness.state().agent(0).region.size();
  const auto size_before_1 = harness.state().agent(1).region.size();

  harness.context().queueEncounter(0, 1);
  harness.tickFor(0);

  EXPECT_GT(harness.state().agent(0).region.size(), size_before_0);
  EXPECT_GT(harness.state().agent(1).region.size(), size_before_1);
}

TEST(NegotiationSubtree, TakasDaliOrtakTaramaDalindanOncelikli)
{
  // Fallback sirasi plandaki sirayla ayni olmali.
  auto config = baseConfig();
  config.sim.joint_scan_threshold = 0.01;
  NegotiationHarness harness(config);

  const auto boundary = AreaSwapNegotiator::boundaryCells(harness.state(), 0, 1);
  for (const int cell_id : boundary) {
    harness.state().interest().deposit(cell_id, 1.0);
  }
  scanFraction(&harness.state(), 0, 0.8);   // dengesizlik de yarat

  harness.context().queueEncounter(0, 1);
  harness.tickFor(0);

  // Takas kabul edildiyse ortak tarama HIC denenmemeli.
  if (harness.counters().swaps_applied > 0) {
    EXPECT_EQ(harness.counters().joint_scans, 0);
  }
}

TEST(NegotiationSubtree, DroneSayisindanBagimsizCalisir)
{
  // Plan: "drone sayisindan bagimsiz, tek basina dogrulanabilir olmali".
  for (const int n : {2, 3, 5, 8}) {
    NegotiationHarness harness(baseConfig(n));
    harness.context().queueEncounter(0, 1);
    EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::SUCCESS) << "N=" << n;
    EXPECT_EQ(harness.counters().status_exchanges, 1) << "N=" << n;
  }
}

TEST(NegotiationSubtree, FarkliAjanlarKendiKuyruklariniIsler)
{
  NegotiationHarness harness(baseConfig(5));
  harness.context().queueEncounter(0, 1);
  harness.context().queueEncounter(2, 3);

  EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::SUCCESS);
  EXPECT_FALSE(harness.context().hasPendingEncounter(0));
  EXPECT_TRUE(harness.context().hasPendingEncounter(2));

  EXPECT_EQ(harness.tickFor(2), BT::NodeStatus::SUCCESS);
  EXPECT_FALSE(harness.context().hasPendingEncounter(2));
  EXPECT_EQ(harness.counters().status_exchanges, 2);
}

TEST(NegotiationSubtree, AyniOrtakIkiKezKuyruklanmaz)
{
  NegotiationHarness harness(baseConfig());
  harness.context().queueEncounter(0, 1);
  harness.context().queueEncounter(0, 1);

  EXPECT_EQ(harness.tickFor(0), BT::NodeStatus::SUCCESS);
  EXPECT_FALSE(harness.context().hasPendingEncounter(0));
}
