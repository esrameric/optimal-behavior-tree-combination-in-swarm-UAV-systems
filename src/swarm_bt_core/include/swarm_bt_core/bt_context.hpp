#ifndef SWARM_BT_CORE__BT_CONTEXT_HPP_
#define SWARM_BT_CORE__BT_CONTEXT_HPP_

#include <deque>
#include <map>

#include "swarm_bt_core/area_swap.hpp"
#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/swarm_state.hpp"

namespace swarm_bt_core
{

/// BT dugumlerinin artirdigi sayaclar (Bolum 6 metriklerinin kaynagi).
struct BtCounters
{
  int status_exchanges{0};    ///< durum_bilgisi_degis() cagrisi
  int swap_proposals{0};      ///< kurulan takas teklifi
  int swaps_applied{0};       ///< kabul edilip uygulanan takas
  int joint_scans{0};         ///< joint_scan_baslat() cagrisi
  int info_shares{0};         ///< sadece_bilgi_paylas_ve_devam_et() cagrisi
  int shared_cell_updates{0};  ///< karsi taraftan ogrenilen yeni hucre bilgisi
  int leader_elections{0};    ///< P2b gecici lider secimi
  int allocations{0};         ///< baslangic alan atamasi
  int scan_steps{0};          ///< tamamlanan tarama adimi (hucre)
  int orphan_transfers{0};    ///< devralinan sahipsiz hucre
  int central_dispatches{0};  ///< P2a merkezi hedef dagitimi
  int coordination_messages{0};  ///< iletisim yuku
};

/// BT dugumlerinin paylastigi calisma baglami.
///
/// Dugumler bu yapiyi kayit sirasinda yakalar; blackboard'da yalnizca ajana
/// OZGU degerler (agent_id, peer_id) tutulur. Boylece ayni dugum kodu hem
/// Faz 1 hafif simulatorde hem Faz 2 Gazebo koşusunda calisir -- degisen tek
/// sey SwarmState icindeki pozisyonlarin kaynagidir (plan Bolum 7).
struct BtContext
{
  SwarmState * state{nullptr};
  const ExperimentConfig * config{nullptr};
  AreaSwapNegotiator * negotiator{nullptr};

  /// Ajan basina islenmeyi bekleyen karsilasma ortaklari.
  /// Karsilasma tespiti (EncounterDetector) buraya yazar, BT buradan okur.
  std::map<int, std::deque<int>> pending_peers;

  BtCounters counters{};

  /// Ajana yeni bir karsilasma ortagi kuyruklar.
  void queueEncounter(int agent_id, int peer_id);

  /// Ajanin bekleyen karsilasmasi var mi.
  bool hasPendingEncounter(int agent_id) const;

  /// Bekleyen ilk ortagi alir ve kuyruktan cikarir; yoksa -1.
  int popPendingPeer(int agent_id);

  void clearPending();
};

/// Blackboard anahtarlari.
namespace bt_keys
{
constexpr const char * kAgentId = "agent_id";
constexpr const char * kPeerId = "peer_id";
}  // namespace bt_keys

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__BT_CONTEXT_HPP_
