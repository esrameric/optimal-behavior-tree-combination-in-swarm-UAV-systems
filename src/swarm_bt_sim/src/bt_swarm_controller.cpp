#include "swarm_bt_sim/bt_swarm_controller.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <swarm_bt_core/bt_nodes.hpp>

namespace swarm_bt_sim
{

std::string defaultBtXmlDir()
{
  return (std::filesystem::path(
           ament_index_cpp::get_package_share_directory("swarm_bt_core")) / "bt_xml").string();
}

BtSwarmController::BtSwarmController(
  const swarm_bt_core::ExperimentConfig & config,
  swarm_bt_core::SwarmState * state,
  swarm_bt_core::AreaSwapNegotiator * negotiator,
  const std::string & xml_dir)
: config_(config)
{
  if (state == nullptr || negotiator == nullptr) {
    throw std::invalid_argument("BtSwarmController: durum ve muzakereci bos olamaz");
  }
  context_.state = state;
  context_.config = &config_;
  context_.negotiator = negotiator;

  swarm_bt_core::registerSwarmNodes(&factory_, &context_);

  const std::filesystem::path directory(xml_dir);
  const auto negotiation = directory / "negotiation_subtree.xml";
  const auto main_xml = directory / config.btXmlFileName();
  if (!std::filesystem::exists(negotiation) || !std::filesystem::exists(main_xml)) {
    throw std::invalid_argument("BtSwarmController: BT XML dosyalari bulunamadi: " + xml_dir);
  }
  factory_.registerBehaviorTreeFromFile(negotiation.string());
  factory_.registerBehaviorTreeFromFile(main_xml.string());

  const auto tree_id = swarm_bt_core::mainTreeIdFor(config.p4);
  single_tree_ = config.p4 == swarm_bt_core::BtArchitecture::kCentral;

  const int tree_count = single_tree_ ? 1 : state->agentCount();
  for (int i = 0; i < tree_count; ++i) {
    auto blackboard = BT::Blackboard::create();
    blackboard->set(swarm_bt_core::bt_keys::kAgentId, single_tree_ ? 0 : i);
    blackboard->set(swarm_bt_core::bt_keys::kPeerId, -1);
    trees_.push_back(factory_.createTree(tree_id, blackboard));
  }
}

int BtSwarmController::centralViewpoint() const
{
  for (const auto & agent : context_.state->agents()) {
    if (agent.alive && context_.hasPendingEncounter(agent.id)) {
      return agent.id;
    }
  }
  for (const auto & agent : context_.state->agents()) {
    if (agent.alive) {
      return agent.id;
    }
  }
  return 0;
}

void BtSwarmController::tick()
{
  if (single_tree_) {
    // P4a: tek agac, suru adina. Hangi ajanin gozunden bakilacagi her tick
    // yeniden secilir; merkez tum ajanlarin durumunu zaten goruyor.
    trees_.front().rootBlackboard()->set(
      swarm_bt_core::bt_keys::kAgentId, centralViewpoint());
    trees_.front().tickOnce();
    return;
  }

  for (auto & tree : trees_) {
    tree.tickOnce();
  }
}

void BtSwarmController::queueEncounter(int agent_id, int peer_id)
{
  context_.queueEncounter(agent_id, peer_id);
}

bool BtSwarmController::hasPendingEncounter(int agent_id) const
{
  return context_.hasPendingEncounter(agent_id);
}

}  // namespace swarm_bt_sim
