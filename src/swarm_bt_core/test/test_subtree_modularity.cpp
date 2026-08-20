// Plan Bolum 7: negotiation alt-agaci AYRI bir .xml dosyasi olarak tutulur ve
// ana agaca subtree include mekanizmasiyla eklenir.
//
// Amaci plan su cumleyle koyuyor: "bu, negotiation mantigini DEGISTIRMEDEN
// farkli P2/P3/P4 kombinasyonlariyla test etmeni kolaylastirir". Bu dosya o
// ozelligi sinar: alt-agac tek bir yerde tanimlidir, uc P4 varyanti da onu
// referansla cagirir, ve alt-agac degistirildiginde P4 dosyalarina
// DOKUNULMADAN ucunun de davranisi degisir.
#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "swarm_bt_core/bt_node_manifest.hpp"

#ifndef SWARM_BT_XML_DIR
#error "SWARM_BT_XML_DIR tanimli degil (CMakeLists.txt'e bakin)"
#endif

namespace
{

const std::vector<std::string> & p4Files()
{
  static const std::vector<std::string> kFiles = {
    "bt_central.xml", "bt_distributed.xml", "bt_event_driven.xml"};
  return kFiles;
}

std::filesystem::path xmlDir()
{
  return std::filesystem::path(SWARM_BT_XML_DIR);
}

std::string readFile(const std::filesystem::path & path)
{
  std::ifstream file(path);
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

/// Dugumleri stub olarak kaydeder.
///
/// \p failing icindeki kimlikler FAILURE, digerleri SUCCESS doner. Alt-agaca
/// GERCEKTEN ulasan bir akis kurmak icin gerekli: butun kosullar SUCCESS
/// donerse bazi agaclarda gorev dali erken bitiyor ve negotiation'a hic
/// gelinmiyor (olculdu).
void registerStubs(
  BT::BehaviorTreeFactory * factory, const std::set<std::string> & failing = {})
{
  for (const auto & spec : swarm_bt_core::swarmNodeManifest()) {
    const bool fails = failing.count(spec.id) > 0;
    const auto status = fails ? BT::NodeStatus::FAILURE : BT::NodeStatus::SUCCESS;
    if (spec.type == BT::NodeType::CONDITION) {
      factory->registerSimpleCondition(spec.id, [status](BT::TreeNode &) {return status;});
    } else {
      factory->registerSimpleAction(spec.id, [status](BT::TreeNode &) {return status;});
    }
  }
}

/// Stub'lari kaydeder ve her dugumun kac kez tiklendigini \p ticks icinde sayar.
void registerCountingStubs(
  BT::BehaviorTreeFactory * factory, std::map<std::string, int> * ticks,
  const std::set<std::string> & failing = {})
{
  for (const auto & spec : swarm_bt_core::swarmNodeManifest()) {
    (*ticks)[spec.id] = 0;
    const bool fails = failing.count(spec.id) > 0;
    const auto status = fails ? BT::NodeStatus::FAILURE : BT::NodeStatus::SUCCESS;
    const std::string id = spec.id;
    const auto functor = [ticks, id, status](BT::TreeNode &) {
        ++(*ticks)[id];
        return status;
      };
    if (spec.type == BT::NodeType::CONDITION) {
      factory->registerSimpleCondition(spec.id, functor);
    } else {
      factory->registerSimpleAction(spec.id, functor);
    }
  }
}

/// Negotiation dalina ulasmak icin gereken stub ayari.
///
///   IsCoverageComplete FAILURE -> gorev dali erken bitmesin
///   IsBoundaryPheromoneHigh FAILURE -> olay-gudumlu agacta ortak tarama dali
///                                      negotiation'in basarisizligini maskelemesin
const std::set<std::string> & reachNegotiationStubs()
{
  static const std::set<std::string> kFailing = {
    "IsCoverageComplete", "IsBoundaryPheromoneHigh"};
  return kFailing;
}

std::string treeIdFor(const std::string & file_name)
{
  if (file_name == "bt_central.xml") {
    return "CentralSwarmTree";
  }
  if (file_name == "bt_distributed.xml") {
    return "DistributedAgentTree";
  }
  return "EventDrivenAgentTree";
}

}  // namespace

TEST(SubtreeModularity, AltAgacKendiDosyasindaTanimli)
{
  const auto path = xmlDir() / "negotiation_subtree.xml";
  ASSERT_TRUE(std::filesystem::exists(path)) << "aranan: " << path;
  const auto content = readFile(path);
  EXPECT_NE(content.find("<BehaviorTree ID=\"NegotiationSubtree\">"), std::string::npos);
}

TEST(SubtreeModularity, P4DosyalariAltAgaciYALNIZCAReferansla)
{
  // Alt-agac govdesi P4 dosyalarina KOPYALANMAMALI; aksi halde negotiation
  // mantigini degistirmek uc dosyaya birden dokunmayi gerektirirdi.
  for (const auto & file_name : p4Files()) {
    const auto content = readFile(xmlDir() / file_name);
    EXPECT_NE(content.find("<SubTree ID=\"NegotiationSubtree\""), std::string::npos)
      << file_name << " alt-agaci referansla cagirmiyor";
    EXPECT_EQ(content.find("<BehaviorTree ID=\"NegotiationSubtree\">"), std::string::npos)
      << file_name << " alt-agac govdesini KOPYALAMIS";
  }
}

TEST(SubtreeModularity, AltAgacTanimiTekYerdeBulunur)
{
  int definitions = 0;
  for (const auto & entry : std::filesystem::directory_iterator(xmlDir())) {
    if (entry.path().extension() != ".xml") {
      continue;
    }
    if (readFile(entry.path()).find("<BehaviorTree ID=\"NegotiationSubtree\">") !=
      std::string::npos)
    {
      ++definitions;
    }
  }
  EXPECT_EQ(definitions, 1) << "alt-agac birden fazla dosyada tanimli";
}

TEST(SubtreeModularity, P4DosyalarinaDokunmadanAltAgacDegistirilebilir)
{
  // Planin vaadi: negotiation mantigi degisince P2/P3/P4 kombinasyonlarina
  // dokunmak gerekmez.
  //
  // Kanit, agacin DONUS DEGERI uzerinden kurulamaz: olay-gudumlu agac her
  // durumda taramaya duşup SUCCESS doner ve alt-agac degisimini maskeler
  // (olculdu). Bunun yerine YAN ETKI sayilir -- iki farkli alt-agac surumu
  // farkli dugumleri tikliyor mu?
  const char * kSwapSubtree =
    R"(<root BTCPP_format="4">
         <BehaviorTree ID="NegotiationSubtree">
           <ProposeAreaSwap/>
         </BehaviorTree>
       </root>)";
  const char * kJointScanSubtree =
    R"(<root BTCPP_format="4">
         <BehaviorTree ID="NegotiationSubtree">
           <StartJointScan/>
         </BehaviorTree>
       </root>)";

  for (const auto & file_name : p4Files()) {
    const auto before = readFile(xmlDir() / file_name);

    auto tickAndCount = [&](const char * subtree_xml) {
        std::map<std::string, int> ticks;
        BT::BehaviorTreeFactory factory;
        registerCountingStubs(&factory, &ticks, reachNegotiationStubs());
        factory.registerBehaviorTreeFromText(subtree_xml);
        factory.registerBehaviorTreeFromFile((xmlDir() / file_name).string());
        auto tree = factory.createTree(treeIdFor(file_name));
        tree.tickOnce();
        return ticks;
      };

    const auto with_swap = tickAndCount(kSwapSubtree);
    const auto with_joint_scan = tickAndCount(kJointScanSubtree);

    // P4 dosyasinin kendisi degismedi.
    EXPECT_EQ(readFile(xmlDir() / file_name), before) << file_name;

    // Ama alt-agacin icerigi agaca yansidi.
    EXPECT_GT(with_swap.at("ProposeAreaSwap"), 0)
      << file_name << ": alt-agactaki ProposeAreaSwap tiklenmedi";
    EXPECT_EQ(with_swap.at("StartJointScan"), 0)
      << file_name << ": alt-agacta olmayan dugum tiklendi";

    EXPECT_GT(with_joint_scan.at("StartJointScan"), 0)
      << file_name << ": alt-agactaki StartJointScan tiklenmedi";
    EXPECT_EQ(with_joint_scan.at("ProposeAreaSwap"), 0)
      << file_name << ": alt-agacta olmayan dugum tiklendi";
  }
}

TEST(SubtreeModularity, UcVaryantAyniAltAgacKimliginiKullanir)
{
  // Ayni kimlik olmasa, alt-agac "tekrar kullanilabilir" olmazdi.
  for (const auto & file_name : p4Files()) {
    const auto content = readFile(xmlDir() / file_name);
    EXPECT_NE(content.find("NegotiationSubtree"), std::string::npos) << file_name;
  }
}

TEST(SubtreeModularity, AltAgacBlackboardPaylasimiIcinAutoremapKullanir)
{
  // agent_id ve peer_id ana agactan alt-agaca gecebilmeli; BT.CPP 4'te
  // alt-agac blackboard'u varsayilan olarak IZOLEDIR.
  for (const auto & file_name : p4Files()) {
    const auto content = readFile(xmlDir() / file_name);
    const auto position = content.find("<SubTree ID=\"NegotiationSubtree\"");
    ASSERT_NE(position, std::string::npos) << file_name;
    const auto tag_end = content.find('>', position);
    const auto tag = content.substr(position, tag_end - position);
    EXPECT_NE(tag.find("_autoremap=\"true\""), std::string::npos)
      << file_name << " alt-agac cagrisinda _autoremap yok: blackboard izole kalir";
  }
}
