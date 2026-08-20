#ifndef SWARM_BT_SIM__BT_SWARM_CONTROLLER_HPP_
#define SWARM_BT_SIM__BT_SWARM_CONTROLLER_HPP_

#include <behaviortree_cpp/bt_factory.h>

#include <memory>
#include <string>
#include <vector>

#include <swarm_bt_core/area_swap.hpp>
#include <swarm_bt_core/bt_context.hpp>
#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_core/swarm_state.hpp>

namespace swarm_bt_sim
{

/// P4 secimine gore BT agac(lar)ini kurar ve her simulasyon tick'inde tikler.
///
/// Agac SAYISI P4'e baglidir ve varyantlar arasindaki asil yapisal fark budur:
///   P4a tek merkezi BT   -> TEK agac, tum suru icin
///   P4b ozdes dagitik BT -> ajan basina bir agac (hepsi ayni XML)
///   P4c olay-gudumlu     -> ajan basina bir agac (reaktif dugumlerle)
///
/// BT yalnizca KARAR verir: hedef hucreyi secer, muzakereyi yurutur. Hareketi
/// ucus katmani (KinematicSim) yapar. Bu ayrim sayesinde ayni BT kodu Faz 2'de
/// Gazebo ile de calisir (plan Bolum 7).
class BtSwarmController
{
public:
  BtSwarmController(
    const swarm_bt_core::ExperimentConfig & config,
    swarm_bt_core::SwarmState * state,
    swarm_bt_core::AreaSwapNegotiator * negotiator,
    const std::string & xml_dir);

  /// Tum agaclari bir kez tikler.
  void tick();

  /// Karsilasmayi ajanin muzakere kuyruguna yazar.
  void queueEncounter(int agent_id, int peer_id);

  bool hasPendingEncounter(int agent_id) const;

  const swarm_bt_core::BtCounters & counters() const {return context_.counters;}
  swarm_bt_core::BtContext & context() {return context_;}

  /// Kurulan agac sayisi (P4a icin 1, digerleri icin ajan sayisi).
  std::size_t treeCount() const {return trees_.size();}

private:
  /// P4a'da tek agac hangi ajanin gozunden tiklenecek: bekleyen karsilasmasi
  /// olan ilk ajan, yoksa 0.
  int centralViewpoint() const;

  swarm_bt_core::ExperimentConfig config_;
  swarm_bt_core::BtContext context_;
  BT::BehaviorTreeFactory factory_;
  std::vector<BT::Tree> trees_;
  bool single_tree_{false};
};

/// Kurulu paketten bt_xml dizininin yolunu bulur.
std::string defaultBtXmlDir();

}  // namespace swarm_bt_sim

#endif  // SWARM_BT_SIM__BT_SWARM_CONTROLLER_HPP_
