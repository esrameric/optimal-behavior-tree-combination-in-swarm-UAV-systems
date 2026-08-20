#include "swarm_bt_core/bt_nodes.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "swarm_bt_core/region_allocator.hpp"

namespace swarm_bt_core
{

namespace
{

// TreeNode::config() korumali oldugu icin ajan/ortak kimlikleri PORT uzerinden
// okunur. Portlarin varsayilan degeri blackboard anahtarina yonlendirir
// ("{agent_id}"), boylece XML tarafinda hicbir port yazmaya gerek kalmaz.
BT::PortsList agentPorts()
{
  return {BT::InputPort<int>(bt_keys::kAgentId, "{agent_id}", "ajan kimligi")};
}

BT::PortsList peerPorts()
{
  return {
    BT::InputPort<int>(bt_keys::kAgentId, "{agent_id}", "ajan kimligi"),
    BT::InputPort<int>(bt_keys::kPeerId, "{peer_id}", "muzakere ortagi")};
}

BT::PortsList exchangePorts()
{
  return {
    BT::InputPort<int>(bt_keys::kAgentId, "{agent_id}", "ajan kimligi"),
    BT::BidirectionalPort<int>(bt_keys::kPeerId, "{peer_id}", "muzakere ortagi")};
}

int agentIdOf(BT::TreeNode & node)
{
  const auto value = node.getInput<int>(bt_keys::kAgentId);
  if (!value) {
    throw BT::RuntimeError("BT dugumu agent_id okuyamadi: ", value.error());
  }
  return value.value();
}

int peerIdOf(BT::TreeNode & node)
{
  const auto value = node.getInput<int>(bt_keys::kPeerId);
  return value ? value.value() : -1;
}

void setPeerId(BT::TreeNode & node, int peer_id)
{
  node.setOutput(bt_keys::kPeerId, peer_id);
}

BT::NodeStatus toStatus(bool ok)
{
  return ok ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
}

/// Ajanin kendi bolgesinde, KENDI BILGISINE gore taranmamis siradaki hucrenin
/// bolge icindeki indeksi; kalmadiysa -1.
///
/// Arama next_waypoint'ten baslar ama SARMALANIR (bastan devam eder). Yalnizca
/// ileri aramak kirilgan bir degismezdi: bolge, takas / sahipsiz alan devralma
/// / ortak tarama sonrasi UCUS SIRASINDA yeniden siralaniyor ve halihazirdaki
/// tarama konumundan ONCEYE hucre duşebiliyor. Olculen sonuc: bir hucre
/// erisilemez kaliyor, kapsama hicbir zaman tamamlanmiyor ve koşu zaman
/// sinirina dayaniyordu.
int nextUnknownCell(const SwarmState & state, int agent_id)
{
  const auto & agent = state.agent(agent_id);
  const std::size_t size = agent.region.size();
  if (size == 0) {
    return -1;
  }
  const std::size_t start = std::min(agent.next_waypoint, size);
  for (std::size_t offset = 0; offset < size; ++offset) {
    const std::size_t index = (start + offset) % size;
    if (!state.knowsVisited(agent_id, agent.region[index])) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

/// ScanNextCell - plan Bolum 2.1 adim 3-4.
///
/// BT "su hucreye git" der ve varilana kadar RUNNING doner; ucus katmani
/// (KinematicSim) hareketi yurutur. RUNNING donebilmesi sart: olay-gudumlu
/// agacin (P4c) calisan bir taramayi KESEBILMESI buna bagli.
class ScanNextCell : public BT::StatefulActionNode
{
public:
  ScanNextCell(const std::string & name, const BT::NodeConfig & config, BtContext * context)
  : BT::StatefulActionNode(name, config), context_(context) {}

  static BT::PortsList providedPorts() {return agentPorts();}

  BT::NodeStatus onStart() override
  {
    auto & state = *context_->state;
    const int agent_id = agentIdOf(*this);
    auto & agent = state.agent(agent_id);

    if (!agent.alive) {
      return BT::NodeStatus::FAILURE;
    }

    // Hedef zaten atanmissa (onceki tick'te kesilmis olabilir) devam et.
    if (agent.target_cell >= 0 && !state.knowsVisited(agent_id, agent.target_cell)) {
      return BT::NodeStatus::RUNNING;
    }

    const int index = nextUnknownCell(state, agent_id);
    if (index < 0) {
      agent.target_cell = -1;
      return BT::NodeStatus::FAILURE;   // kendi bolgesinde is kalmadi
    }
    agent.next_waypoint = static_cast<std::size_t>(index);
    agent.target_cell = agent.region[static_cast<std::size_t>(index)];
    agent.at_target = false;
    return BT::NodeStatus::RUNNING;
  }

  BT::NodeStatus onRunning() override
  {
    auto & state = *context_->state;
    const int agent_id = agentIdOf(*this);
    auto & agent = state.agent(agent_id);

    if (!agent.alive) {
      return BT::NodeStatus::FAILURE;
    }
    if (agent.at_target) {
      agent.at_target = false;
      ++agent.next_waypoint;
      ++context_->counters.scan_steps;

      // Siradaki hedef AYNI tick'te secilir. Aksi halde ucus katmani bir
      // sonraki tick'te hedefsiz kalir ve her hucre basina bir tick bosa
      // gider (olculdu: gorev suresi yaklasik iki katina cikiyordu).
      const int index = nextUnknownCell(state, agent_id);
      agent.target_cell =
        (index < 0) ? -1 : agent.region[static_cast<std::size_t>(index)];
      if (index >= 0) {
        agent.next_waypoint = static_cast<std::size_t>(index);
      }
      return BT::NodeStatus::SUCCESS;
    }
    return BT::NodeStatus::RUNNING;
  }

  void onHalted() override
  {
    // Hedef bilincli olarak KORUNUR: olay islendikten sonra ajan kaldigi
    // yerden devam eder, bastan planlamaz.
  }

private:
  BtContext * context_;
};

}  // namespace

std::string mainTreeIdFor(BtArchitecture architecture)
{
  switch (architecture) {
    case BtArchitecture::kCentral: return "CentralSwarmTree";
    case BtArchitecture::kDistributed: return "DistributedAgentTree";
    case BtArchitecture::kEventDriven: return "EventDrivenAgentTree";
  }
  throw std::invalid_argument("mainTreeIdFor: bilinmeyen P4 degeri");
}

void registerSwarmNodes(BT::BehaviorTreeFactory * factory, BtContext * context)
{
  if (factory == nullptr || context == nullptr) {
    throw std::invalid_argument("registerSwarmNodes: fabrika ve baglam bos olamaz");
  }

  // --- Gorev yurutme (Bolum 2.1) ---

  factory->registerSimpleCondition(
    "IsAgentAlive", [context](BT::TreeNode & node) {
      return toStatus(context->state->agent(agentIdOf(node)).alive);
    }, agentPorts());

  factory->registerSimpleCondition(
    "IsCoverageComplete", [context](BT::TreeNode & node) {
      return toStatus(nextUnknownCell(*context->state, agentIdOf(node)) < 0);
    }, agentPorts());

  factory->registerNodeType<ScanNextCell>("ScanNextCell", context);

  // --- Baslangic atamasi (Bolum 2.1 adim 2, P3) ---

  factory->registerSimpleCondition(
    "IsAllocationDone", [context](BT::TreeNode & node) {
      return toStatus(!context->state->agent(agentIdOf(node)).region.empty());
    }, agentPorts());

  factory->registerSimpleAction(
    "AllocateInitialRegions", [context](BT::TreeNode &) {
      // Atama koşu basinda makeSwarmState() icinde P3'e gore yapilir; bu dugum
      // agacin akisinda o adimi temsil eder ve idempotenttir.
      ++context->counters.allocations;
      return BT::NodeStatus::SUCCESS;
    }, {});

  // --- Karsilasma ve negotiation (Bolum 2.2) ---

  factory->registerSimpleCondition(
    "HasPendingEncounter", [context](BT::TreeNode & node) {
      return toStatus(context->hasPendingEncounter(agentIdOf(node)));
    }, agentPorts());

  factory->registerSimpleAction(
    "ExchangeStatus", [context](BT::TreeNode & node) {
      // durum_bilgisi_degis(): kalan_alan_yuzdesi, batarya, oncelik_puani
      // karsilikli paylasilir. Bekleyen ortak burada kuyruktan ALINIR; boylece
      // bir karsilasma tam olarak bir muzakere uretir.
      const int agent_id = agentIdOf(node);
      const int peer_id = context->popPendingPeer(agent_id);
      if (peer_id < 0) {
        return BT::NodeStatus::FAILURE;
      }
      setPeerId(node, peer_id);
      ++context->counters.status_exchanges;
      context->counters.coordination_messages += 2;

      // Dogrudan mesaj (P5a) yoksa durum bilgisi akmaz.
      if (!context->config->p5.direct_message) {
        return BT::NodeStatus::FAILURE;
      }
      context->counters.shared_cell_updates +=
      context->state->shareKnowledge(agent_id, peer_id);
      return BT::NodeStatus::SUCCESS;
    }, exchangePorts());

  factory->registerSimpleCondition(
    "IsAreaImbalanceAboveThreshold", [context](BT::TreeNode & node) {
      const int peer_id = peerIdOf(node);
      if (peer_id < 0) {
        return BT::NodeStatus::FAILURE;
      }
      return toStatus(
        context->negotiator->imbalanceAboveThreshold(
          *context->state, agentIdOf(node), peer_id));
    }, peerPorts());

  factory->registerSimpleAction(
    "ProposeAreaSwap", [context](BT::TreeNode & node) {
      // alan_takasi_teklif_et(): teklif edilen taraf kendi faydasini hesaplar.
      const int peer_id = peerIdOf(node);
      if (peer_id < 0) {
        return BT::NodeStatus::FAILURE;
      }
      const auto proposal =
      context->negotiator->buildProposal(*context->state, agentIdOf(node), peer_id);
      if (!proposal.has_value()) {
        return BT::NodeStatus::FAILURE;
      }
      ++context->counters.swap_proposals;
      if (!proposal->reducesTotalDistance()) {
        return BT::NodeStatus::FAILURE;   // teklif reddedildi
      }
      context->negotiator->apply(context->state, *proposal);
      ++context->counters.swaps_applied;
      return BT::NodeStatus::SUCCESS;
    }, peerPorts());

  factory->registerSimpleCondition(
    "IsBoundaryPheromoneHigh", [context](BT::TreeNode & node) {
      // sinir_bolgesinde_feromon_yuksek()
      const int peer_id = peerIdOf(node);
      if (peer_id < 0) {
        return BT::NodeStatus::FAILURE;
      }
      const double pheromone = AreaSwapNegotiator::boundaryPheromone(
        *context->state, agentIdOf(node), peer_id);
      return toStatus(pheromone > context->config->sim.joint_scan_threshold);
    }, peerPorts());

  factory->registerSimpleAction(
    "StartJointScan", [context](BT::TreeNode & node) {
      // joint_scan_baslat()
      const int peer_id = peerIdOf(node);
      if (peer_id < 0) {
        return BT::NodeStatus::FAILURE;
      }
      const int shared =
      AreaSwapNegotiator::startJointScan(context->state, agentIdOf(node), peer_id);
      if (shared == 0) {
        return BT::NodeStatus::FAILURE;
      }
      ++context->counters.joint_scans;
      return BT::NodeStatus::SUCCESS;
    }, peerPorts());

  factory->registerSimpleAction(
    "ShareInfoAndContinue", [context](BT::TreeNode & node) {
      // sadece_bilgi_paylas_ve_devam_et(): stigmerji senkronize edilir,
      // yollarina devam. Hicbir esik asilmadiginda calisan varsayilan dal.
      const int peer_id = peerIdOf(node);
      if (peer_id < 0) {
        return BT::NodeStatus::FAILURE;
      }
      ++context->counters.info_shares;
      const int transferred = AreaSwapNegotiator::distributeOrphans(
        context->state, agentIdOf(node), peer_id);
      context->counters.orphan_transfers += transferred;
      return BT::NodeStatus::SUCCESS;
    }, peerPorts());

  // --- Koordinasyon mimarisi (P2) ---

  factory->registerSimpleAction(
    "ElectTemporaryLeader", [context](BT::TreeNode &) {
      ++context->counters.leader_elections;
      return BT::NodeStatus::SUCCESS;
    }, {});

  factory->registerSimpleAction(
    "CollectSwarmState", [context](BT::TreeNode &) {
      // P2a: tum ajanlar merkeze rapor verir.
      context->counters.coordination_messages += context->state->agentCount();
      return BT::NodeStatus::SUCCESS;
    }, {});

  factory->registerSimpleAction(
    "DispatchWaypoints", [context](BT::TreeNode &) {
      // P2a: merkez her ajana siradaki hedefini atar.
      auto & state = *context->state;
      for (auto & agent : state.agents()) {
        if (!agent.alive) {
          continue;
        }
        if (agent.at_target) {
          agent.at_target = false;
          agent.target_cell = -1;
          ++agent.next_waypoint;
          ++context->counters.scan_steps;
        }
        if (agent.target_cell < 0) {
          const int index = nextUnknownCell(state, agent.id);
          if (index >= 0) {
            agent.next_waypoint = static_cast<std::size_t>(index);
            agent.target_cell = agent.region[static_cast<std::size_t>(index)];
            agent.at_target = false;
          }
        }
      }
      ++context->counters.central_dispatches;
      return BT::NodeStatus::SUCCESS;
    }, {});
}

BT::Tree createSwarmTree(
  BT::BehaviorTreeFactory * factory,
  const std::string & xml_dir,
  const std::string & main_xml,
  int agent_id)
{
  if (factory == nullptr) {
    throw std::invalid_argument("createSwarmTree: fabrika bos olamaz");
  }
  const std::filesystem::path directory(xml_dir);
  const auto negotiation = directory / "negotiation_subtree.xml";
  if (!std::filesystem::exists(negotiation)) {
    throw std::invalid_argument("createSwarmTree: negotiation_subtree.xml bulunamadi: " + xml_dir);
  }

  auto blackboard = BT::Blackboard::create();
  blackboard->set(bt_keys::kAgentId, agent_id);
  blackboard->set(bt_keys::kPeerId, -1);

  factory->registerBehaviorTreeFromFile(negotiation.string());
  factory->registerBehaviorTreeFromFile((directory / main_xml).string());
  return factory->createTree(
    mainTreeIdFor(
      btArchitectureFromLetter(
        main_xml == "bt_central.xml" ? "a" : (main_xml == "bt_distributed.xml" ? "b" : "c"))),
    blackboard);
}

}  // namespace swarm_bt_core
