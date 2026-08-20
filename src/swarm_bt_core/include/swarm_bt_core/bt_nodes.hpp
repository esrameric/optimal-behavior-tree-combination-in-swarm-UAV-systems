#ifndef SWARM_BT_CORE__BT_NODES_HPP_
#define SWARM_BT_CORE__BT_NODES_HPP_

#include <behaviortree_cpp/bt_factory.h>

#include <string>

#include "swarm_bt_core/bt_context.hpp"

namespace swarm_bt_core
{

/// Manifestteki tum ozel BT dugumlerini fabrikaya kaydeder.
///
/// Dugumler \p context'i yakalar; blackboard'da yalnizca ajana ozgu degerler
/// (agent_id, peer_id) tutulur. Ayni fabrika hem tek bir merkezi agac (P4a)
/// hem de ajan basina birer agac (P4b/P4c) uretmek icin kullanilabilir.
///
/// \p context isaretcisi, uretilen agaclar yasadigi surece gecerli kalmalidir.
void registerSwarmNodes(BT::BehaviorTreeFactory * factory, BtContext * context);

/// bt_xml/ dizinindeki agaclari fabrikaya yukler ve istenen agaci uretir.
///
/// \param xml_dir bt_xml dizini
/// \param main_xml yuklenecek P4 varyanti (orn. "bt_distributed.xml")
/// \param agent_id agaca atanacak ajan kimligi (merkezi agac icin -1)
BT::Tree createSwarmTree(
  BT::BehaviorTreeFactory * factory,
  const std::string & xml_dir,
  const std::string & main_xml,
  int agent_id);

/// P4 secimine karsilik gelen agac kimligi (XML'deki BehaviorTree ID).
std::string mainTreeIdFor(BtArchitecture architecture);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__BT_NODES_HPP_
