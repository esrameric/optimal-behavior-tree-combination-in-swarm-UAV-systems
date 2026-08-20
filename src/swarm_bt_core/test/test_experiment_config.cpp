#include <gtest/gtest.h>

#include <string>

#include "swarm_bt_core/experiment_config.hpp"

using swarm_bt_core::AllocationAlgorithm;
using swarm_bt_core::BtArchitecture;
using swarm_bt_core::CommunicationMechanisms;
using swarm_bt_core::CoordinationArchitecture;
using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::TriggerModel;

TEST(ExperimentConfig, DeneyKimligiPlandakiSemayaUyar)
{
  // Plan Bolum 3: "P2b_P3c_P4c_P5bc_P6c_N3"
  ExperimentConfig config;
  config.p2 = CoordinationArchitecture::kHierarchicalHybrid;
  config.p3 = AllocationAlgorithm::kCbba;
  config.p4 = BtArchitecture::kEventDriven;
  config.p5 = CommunicationMechanisms::fromLetters("bc");
  config.p6 = TriggerModel::kEventDriven;
  config.n_agents = 3;

  EXPECT_EQ(config.experimentId(), "P2b_P3c_P4c_P5bc_P6c_N3");

  config.n_agents = 5;
  EXPECT_EQ(config.experimentId(), "P2b_P3c_P4c_P5bc_P6c_N5");
}

TEST(ExperimentConfig, BaselineKimligiTuretilir)
{
  // Plan Bolum 4 onerilen baseline: P2c + P3c + P4b + P5abc + P6c
  ExperimentConfig config;  // varsayilanlar baseline ile ayni
  EXPECT_EQ(config.experimentId(), "P2c_P3c_P4b_P5abc_P6c_N3");
}

TEST(ExperimentConfig, P4SecimiBtXmlDosyasiniBelirler)
{
  ExperimentConfig config;
  config.p4 = BtArchitecture::kCentral;
  EXPECT_EQ(config.btXmlFileName(), "bt_central.xml");
  config.p4 = BtArchitecture::kDistributed;
  EXPECT_EQ(config.btXmlFileName(), "bt_distributed.xml");
  config.p4 = BtArchitecture::kEventDriven;
  EXPECT_EQ(config.btXmlFileName(), "bt_event_driven.xml");
}

TEST(ExperimentConfig, IletisimMekanizmalariHarfDizisineDonusur)
{
  EXPECT_EQ(CommunicationMechanisms{}.toLetters(), "abc");
  EXPECT_EQ(CommunicationMechanisms::fromLetters("abcd").toLetters(), "abcd");
  EXPECT_EQ(CommunicationMechanisms::fromLetters("b").toLetters(), "b");
  EXPECT_EQ(CommunicationMechanisms::fromLetters("none").toLetters(), "none");
  EXPECT_THROW(CommunicationMechanisms::fromLetters("x"), std::invalid_argument);
}

TEST(ExperimentConfig, EnumHarfDonusumleriTersinirdir)
{
  for (const std::string letter : {"a", "b", "c"}) {
    EXPECT_EQ(toLetter(swarm_bt_core::coordinationFromLetter(letter)), letter);
    EXPECT_EQ(toLetter(swarm_bt_core::allocationFromLetter(letter)), letter);
    EXPECT_EQ(toLetter(swarm_bt_core::btArchitectureFromLetter(letter)), letter);
    EXPECT_EQ(toLetter(swarm_bt_core::triggerModelFromLetter(letter)), letter);
  }
  EXPECT_THROW(swarm_bt_core::coordinationFromLetter("d"), std::invalid_argument);
  EXPECT_THROW(swarm_bt_core::allocationFromLetter("ab"), std::invalid_argument);
}

TEST(ExperimentConfig, YamlGidisDonusKorunur)
{
  ExperimentConfig original;
  original.n_agents = 5;
  original.r_comm = 72.5;
  original.swap_threshold = 0.25;
  original.p2 = CoordinationArchitecture::kCentral;
  original.p3 = AllocationAlgorithm::kContractNet;
  original.p4 = BtArchitecture::kEventDriven;
  original.p5 = CommunicationMechanisms::fromLetters("bd");
  original.p6 = TriggerModel::kPeriodicPolling;
  original.sim.area_side = 500.0;
  original.sim.cell_size = 25.0;
  original.sim.dt = 0.05;
  original.failure.enabled = true;
  original.failure.time = 42.0;
  original.failure.agent_id = 2;
  original.repetitions = 20;
  original.seed = 7;

  const auto restored = ExperimentConfig::fromYamlString(original.toYaml());

  EXPECT_EQ(restored.experimentId(), original.experimentId());
  EXPECT_EQ(restored.n_agents, 5);
  EXPECT_DOUBLE_EQ(restored.r_comm, 72.5);
  EXPECT_DOUBLE_EQ(restored.swap_threshold, 0.25);
  EXPECT_DOUBLE_EQ(restored.sim.area_side, 500.0);
  EXPECT_DOUBLE_EQ(restored.sim.dt, 0.05);
  EXPECT_TRUE(restored.failure.enabled);
  EXPECT_DOUBLE_EQ(restored.failure.time, 42.0);
  EXPECT_EQ(restored.failure.agent_id, 2);
  EXPECT_EQ(restored.repetitions, 20);
  EXPECT_EQ(restored.seed, 7);
}

TEST(ExperimentConfig, EksikAlanlarVarsayilanDegerdeKalir)
{
  // Deney dosyalari yalnizca baseline'dan FARKLI parametreleri tasiyabilmeli
  // (OFAT taramasi bunun uzerine kurulu).
  const auto config = ExperimentConfig::fromYamlString("N: 5\nP4: a\n");
  EXPECT_EQ(config.n_agents, 5);
  EXPECT_EQ(config.p4, BtArchitecture::kCentral);
  EXPECT_DOUBLE_EQ(config.r_comm, ExperimentConfig{}.r_comm);
  EXPECT_DOUBLE_EQ(config.sim.area_side, ExperimentConfig{}.sim.area_side);
}

TEST(ExperimentConfig, GecersizDegerlerReddedilir)
{
  EXPECT_THROW(ExperimentConfig::fromYamlString("N: 0\n"), std::invalid_argument);
  EXPECT_THROW(ExperimentConfig::fromYamlString("r_comm: -1\n"), std::invalid_argument);
  EXPECT_THROW(ExperimentConfig::fromYamlString("esik_degeri: 1.5\n"), std::invalid_argument);
  EXPECT_THROW(ExperimentConfig::fromYamlString("P2: z\n"), std::invalid_argument);
  EXPECT_THROW(
    ExperimentConfig::fromYamlString("sim:\n  cell_size: 1000\n"), std::invalid_argument);
  // r_comm gorev alanindan buyuk olamaz
  EXPECT_THROW(ExperimentConfig::fromYamlString("r_comm: 5000\n"), std::invalid_argument);
}

TEST(ExperimentConfig, OlmayanDosyaHataVerir)
{
  EXPECT_THROW(ExperimentConfig::fromYamlFile("/olmayan/yol.yaml"), std::invalid_argument);
}
