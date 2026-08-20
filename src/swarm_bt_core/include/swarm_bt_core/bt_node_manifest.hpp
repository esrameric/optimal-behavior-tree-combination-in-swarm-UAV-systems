#ifndef SWARM_BT_CORE__BT_NODE_MANIFEST_HPP_
#define SWARM_BT_CORE__BT_NODE_MANIFEST_HPP_

#include <behaviortree_cpp/basic_types.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace swarm_bt_core
{

/// BT XML dosyalarinin referans verdigi ozel dugumlerden birinin bildirimi.
///
/// Manifest, dugum GERCEKLESTIRIMLERINDEN bagimsiz bir bildirimdir: BT XML
/// varyantlarinin yapisal gecerliligi, dugumler yazilmadan once de bu manifest
/// uzerinden dogrulanabilir. Ayrica Groot2 dugum modeli uretimi ve fabrika
/// kaydi da bu tek listeden beslenir.
struct SwarmNodeSpec
{
  /// BT XML'de gecen dugum kimligi.
  std::string id;
  /// Action / Condition ayrimi.
  BT::NodeType type;
  /// Plandaki karsiligi (Turkce terim ya da bolum referansi).
  std::string plan_reference;
};

/// Projenin tum ozel BT dugumlerinin tek kaynagi.
const std::vector<SwarmNodeSpec> & swarmNodeManifest();

/// BT::VerifyXML icin dugum adi -> tur esleme tablosu.
std::unordered_map<std::string, BT::NodeType> swarmNodeTypeMap();

/// Manifestte bildirilen kimlikler arasinda arama.
bool isSwarmNodeDeclared(const std::string & id);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__BT_NODE_MANIFEST_HPP_
