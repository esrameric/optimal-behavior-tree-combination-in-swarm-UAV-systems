#include "swarm_bt_core/bt_node_manifest.hpp"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace swarm_bt_core
{

const std::vector<SwarmNodeSpec> & swarmNodeManifest()
{
  // Dugum kimlikleri Ingilizce, plandaki Turkce karsiliklari yaninda verilmistir.
  static const std::vector<SwarmNodeSpec> kManifest = {
    // --- Gorev yurutme (Bolum 2.1) ---
    {"IsAgentAlive", BT::NodeType::CONDITION,
      "Bolum 2.3: arizalandirilan drone BT'yi FAILURE'a zorlar"},
    {"IsCoverageComplete", BT::NodeType::CONDITION,
      "Bolum 2.1 adim 6: tum alt-bolgeler tarandi mi"},
    {"ScanNextCell", BT::NodeType::ACTION,
      "Bolum 2.1 adim 3-4: lawn-mower waypoint takibi + stigmerji isaretleme"},

    // --- Baslangic atamasi (Bolum 2.1 adim 2, P3) ---
    {"IsAllocationDone", BT::NodeType::CONDITION,
      "Bolum 2.1 adim 2: alt-bolge atamasi yapildi mi"},
    {"AllocateInitialRegions", BT::NodeType::ACTION,
      "Bolum 2.1 adim 2 / P3: statik esit bolme, Contract Net ya da CBBA"},

    // --- Karsilasma ve negotiation (Bolum 2.2) ---
    {"HasPendingEncounter", BT::NodeType::CONDITION,
      "Bolum 2.1 adim 5: comm-range girisi ile tetiklenen karsilasma olayi"},
    {"ExchangeStatus", BT::NodeType::ACTION,
      "durum_bilgisi_degis(): kalan_alan_yuzdesi, batarya, oncelik_puani paylasimi"},
    {"IsAreaImbalanceAboveThreshold", BT::NodeType::CONDITION,
      "kalan_alan_farki > esik_degeri"},
    {"ProposeAreaSwap", BT::NodeType::ACTION,
      "alan_takasi_teklif_et(): teklif edilen taraf faydasini hesaplar, kabul/red"},
    {"IsBoundaryPheromoneHigh", BT::NodeType::CONDITION,
      "sinir_bolgesinde_feromon_yuksek()"},
    {"StartJointScan", BT::NodeType::ACTION,
      "joint_scan_baslat()"},
    {"ShareInfoAndContinue", BT::NodeType::ACTION,
      "sadece_bilgi_paylas_ve_devam_et(): stigmerji senkronize edilir, yola devam"},

    // --- Koordinasyon mimarisi (P2) ---
    {"ElectTemporaryLeader", BT::NodeType::ACTION,
      "P2b: hiyerarsik hibrit mimaride gecici lider secimi"},
    {"CollectSwarmState", BT::NodeType::ACTION,
      "P2a: tum ajanlarin durumunun merkezde toplanmasi"},
    {"DispatchWaypoints", BT::NodeType::ACTION,
      "P2a: hedeflerin merkezden tum ajanlara dagitilmasi"},
  };
  return kManifest;
}

std::unordered_map<std::string, BT::NodeType> swarmNodeTypeMap()
{
  std::unordered_map<std::string, BT::NodeType> types;
  for (const auto & spec : swarmNodeManifest()) {
    types.emplace(spec.id, spec.type);
  }
  return types;
}

bool isSwarmNodeDeclared(const std::string & id)
{
  const auto & manifest = swarmNodeManifest();
  return std::any_of(
    manifest.begin(), manifest.end(),
    [&id](const SwarmNodeSpec & spec) {return spec.id == id;});
}

}  // namespace swarm_bt_core
