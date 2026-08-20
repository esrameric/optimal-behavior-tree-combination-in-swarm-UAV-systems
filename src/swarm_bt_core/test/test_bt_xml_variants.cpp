// P4 BT XML varyantlarinin (bt_central / bt_distributed / bt_event_driven)
// yapisal gecerliligini ve birbirlerinden gercekten farkli olduklarini dogrular.
//
// Dugum GERCEKLESTIRIMLERI henuz yazilmadi; testler manifestten uretilen stub
// dugumlerle calisir. Boylece XML yapisi, dugum mantigindan bagimsiz olarak
// dogrulanabilir -- plan Bolum 0'daki "P4 secenekleri agacin dugum YAPISINI
// degistirir" iddiasinin dogrudan sinanmasi budur.
#include <gtest/gtest.h>

#include <behaviortree_cpp/behavior_tree.h>
#include <behaviortree_cpp/bt_factory.h>

#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "swarm_bt_core/bt_node_manifest.hpp"
#include "swarm_bt_core/experiment_config.hpp"

#ifndef SWARM_BT_XML_DIR
#error "SWARM_BT_XML_DIR tanimli degil (CMakeLists.txt'e bakin)"
#endif

namespace
{

const char * kNegotiationStub =
  R"(<root BTCPP_format="4">
       <BehaviorTree ID="NegotiationSubtree">
         <AlwaysSuccess/>
       </BehaviorTree>
     </root>)";

std::filesystem::path xmlDir()
{
  return std::filesystem::path(SWARM_BT_XML_DIR);
}

/// Manifestteki her dugumu, davranisi sabit bir stub olarak fabrikaya kaydeder.
void registerStubs(BT::BehaviorTreeFactory * factory)
{
  for (const auto & spec : swarm_bt_core::swarmNodeManifest()) {
    if (spec.type == BT::NodeType::CONDITION) {
      factory->registerSimpleCondition(
        spec.id, [](BT::TreeNode &) {return BT::NodeStatus::SUCCESS;});
    } else {
      factory->registerSimpleAction(
        spec.id, [](BT::TreeNode &) {return BT::NodeStatus::SUCCESS;});
    }
  }
}

/// Agactaki tum dugumlerin kayit kimliklerini toplar.
std::set<std::string> nodeIdsOf(const BT::Tree & tree)
{
  std::set<std::string> ids;
  tree.applyVisitor(
    [&ids](const BT::TreeNode * node) {
      ids.insert(node->registrationName());
    });
  return ids;
}

std::set<std::string> loadIds(const std::string & file_name, const std::string & tree_id)
{
  BT::BehaviorTreeFactory factory;
  registerStubs(&factory);
  factory.registerBehaviorTreeFromText(kNegotiationStub);
  factory.registerBehaviorTreeFromFile((xmlDir() / file_name).string());
  const auto tree = factory.createTree(tree_id);
  return nodeIdsOf(tree);
}

struct Variant
{
  std::string file_name;
  std::string tree_id;
};

const std::vector<Variant> & variants()
{
  static const std::vector<Variant> kVariants = {
    {"bt_central.xml", "CentralSwarmTree"},
    {"bt_distributed.xml", "DistributedAgentTree"},
    {"bt_event_driven.xml", "EventDrivenAgentTree"},
  };
  return kVariants;
}

}  // namespace

TEST(BtXmlVariants, UcVaryantDaMevcut)
{
  for (const auto & variant : variants()) {
    EXPECT_TRUE(std::filesystem::exists(xmlDir() / variant.file_name))
      << variant.file_name << " (aranan dizin: " << xmlDir() << ")";
  }
}

TEST(BtXmlVariants, HerVaryantAyristirilipTicklenebiliyor)
{
  for (const auto & variant : variants()) {
    BT::BehaviorTreeFactory factory;
    registerStubs(&factory);
    factory.registerBehaviorTreeFromText(kNegotiationStub);
    ASSERT_NO_THROW(
      factory.registerBehaviorTreeFromFile((xmlDir() / variant.file_name).string()))
      << variant.file_name;

    auto tree = factory.createTree(variant.tree_id);
    EXPECT_NO_THROW(tree.tickOnce()) << variant.file_name;
  }
}

TEST(BtXmlVariants, KullanilanTumOzelDugumlerManifesteBildirilmis)
{
  // Manifeste yazilmamis bir dugum XML'e sizarsa, dugum gerceklestirimleri
  // yazildiginda sessizce eksik kalirdi.
  static const std::set<std::string> kBuiltins = {
    "Sequence", "ReactiveSequence", "Fallback", "ReactiveFallback",
    "Inverter", "AlwaysSuccess", "AlwaysFailure", "SubTree",
  };

  for (const auto & variant : variants()) {
    for (const auto & id : loadIds(variant.file_name, variant.tree_id)) {
      if (kBuiltins.count(id) > 0) {
        continue;
      }
      EXPECT_TRUE(swarm_bt_core::isSwarmNodeDeclared(id))
        << variant.file_name << " icindeki '" << id << "' manifestte yok";
    }
  }
}

TEST(BtXmlVariants, UcVaryantYapisalOlarakFarkli)
{
  // P4 seceneklerinin config ile ifade edilememesinin sebebi: agac yapisi degisiyor.
  const auto central = loadIds("bt_central.xml", "CentralSwarmTree");
  const auto distributed = loadIds("bt_distributed.xml", "DistributedAgentTree");
  const auto event_driven = loadIds("bt_event_driven.xml", "EventDrivenAgentTree");

  EXPECT_NE(central, distributed);
  EXPECT_NE(central, event_driven);
  EXPECT_NE(distributed, event_driven);
}

TEST(BtXmlVariants, MerkeziVaryantSuruDuzeyindeToplaDagitCiftiIcerir)
{
  const auto central = loadIds("bt_central.xml", "CentralSwarmTree");
  EXPECT_EQ(central.count("CollectSwarmState"), 1u);
  EXPECT_EQ(central.count("DispatchWaypoints"), 1u);

  // Bu cift yalnizca merkezi varyanta ozgu olmali.
  for (const auto & file : {"bt_distributed.xml", "bt_event_driven.xml"}) {
    const std::string tree_id =
      std::string(file) == "bt_distributed.xml" ? "DistributedAgentTree" : "EventDrivenAgentTree";
    const auto ids = loadIds(file, tree_id);
    EXPECT_EQ(ids.count("CollectSwarmState"), 0u) << file;
    EXPECT_EQ(ids.count("DispatchWaypoints"), 0u) << file;
  }
}

TEST(BtXmlVariants, OlayGudumluVaryantReaktifKontrolDugumleriKullanir)
{
  // P4c'nin P4b'den farki tam olarak preemption: reaktif dugumler her tick
  // yuksek oncelikli dallari yeniden degerlendirir.
  const auto event_driven = loadIds("bt_event_driven.xml", "EventDrivenAgentTree");
  EXPECT_EQ(event_driven.count("ReactiveFallback"), 1u);
  EXPECT_EQ(event_driven.count("ReactiveSequence"), 1u);

  const auto distributed = loadIds("bt_distributed.xml", "DistributedAgentTree");
  EXPECT_EQ(distributed.count("ReactiveFallback"), 0u);
  EXPECT_EQ(distributed.count("ReactiveSequence"), 0u);
}

TEST(BtXmlVariants, TumVaryantlarNegotiationAltAgaciniCagirir)
{
  // Klockner modulerlik ilkesi: negotiation mantigi tek yerde durur ve
  // P2/P3/P4 kombinasyonlarindan bagimsiz olarak yeniden kullanilir.
  for (const auto & variant : variants()) {
    const auto ids = loadIds(variant.file_name, variant.tree_id);
    EXPECT_GT(ids.count("NegotiationSubtree") + ids.count("SubTree"), 0u)
      << variant.file_name << " negotiation alt-agacini cagirmiyor";
  }
}

TEST(BtXmlVariants, ConfigP4SecimiDogruDosyayiGosterir)
{
  swarm_bt_core::ExperimentConfig config;
  for (const auto & letter_and_file : std::vector<std::pair<std::string, std::string>>{
    {"a", "bt_central.xml"}, {"b", "bt_distributed.xml"}, {"c", "bt_event_driven.xml"}})
  {
    config.p4 = swarm_bt_core::btArchitectureFromLetter(letter_and_file.first);
    EXPECT_EQ(config.btXmlFileName(), letter_and_file.second);
    EXPECT_TRUE(std::filesystem::exists(xmlDir() / config.btXmlFileName()));
  }
}
