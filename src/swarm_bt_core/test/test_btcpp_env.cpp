// Faz 0 kurulum dogrulamasi: colcon test + GTest altyapisinin ayakta oldugunu ve
// BehaviorTree.CPP'nin (v4+) gercekten baglanip agac tickleyebildigini gosterir.
#include <gtest/gtest.h>

#include <behaviortree_cpp/bt_factory.h>

#include <string>

#include "swarm_bt_core/version.hpp"

TEST(BtcppEnv, KutuphaneSurumuDorttenBuyukEsit)
{
  // Plan, BT kutuphanesi olarak yalnizca BehaviorTree.CPP v4+ kullanilmasini sart kosuyor.
  EXPECT_GE(swarm_bt_core::btcppMajorVersion(), 4)
    << "Baglanan BT.CPP surumu: " << swarm_bt_core::btcppVersion();
}

TEST(BtcppEnv, AsgariAgacOlusturulupTicklenebiliyor)
{
  BT::BehaviorTreeFactory factory;
  auto tree = factory.createTreeFromText(
    R"(<root BTCPP_format="4">
         <BehaviorTree ID="Smoke">
           <Sequence>
             <AlwaysSuccess/>
           </Sequence>
         </BehaviorTree>
       </root>)");

  EXPECT_EQ(tree.tickOnce(), BT::NodeStatus::SUCCESS);
}

TEST(BtcppEnv, XmlAyristirmaHatasiIstisnaFirlatiyor)
{
  // Bozuk XML sessizce yutulmamali: ilerideki bt_xml varyantlarinin
  // dogrulanabilir olmasi buna bagli.
  BT::BehaviorTreeFactory factory;
  EXPECT_THROW(
    factory.createTreeFromText(R"(<root BTCPP_format="4"><BehaviorTree ID="X">)"),
    BT::RuntimeError);
}
