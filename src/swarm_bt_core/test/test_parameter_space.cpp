// Plan Bolum 3: N in {3,5} olcek degiskeni ve kombinasyon x olcek cogaltmasi.
#include <gtest/gtest.h>

#include <set>
#include <string>
#include <vector>

#include "swarm_bt_core/parameter_space.hpp"

using swarm_bt_core::ExperimentConfig;
using swarm_bt_core::combinationId;
using swarm_bt_core::scaleValues;
using swarm_bt_core::withScaleVariants;

TEST(ParameterSpace, OlcekDegerleriPlandakiGibi)
{
  EXPECT_EQ(scaleValues(), (std::vector<int>{3, 5}));
}

TEST(ParameterSpace, KombinasyonHerOlcekIcinCogaltilir)
{
  ExperimentConfig base;
  const auto variants = withScaleVariants(base);

  ASSERT_EQ(variants.size(), 2u);
  EXPECT_EQ(variants[0].n_agents, 3);
  EXPECT_EQ(variants[1].n_agents, 5);
}

TEST(ParameterSpace, CogaltmaYalnizcaNiDegistirir)
{
  ExperimentConfig base;
  base.p2 = swarm_bt_core::CoordinationArchitecture::kHierarchicalHybrid;
  base.p4 = swarm_bt_core::BtArchitecture::kEventDriven;
  base.r_comm = 55.0;
  base.swap_threshold = 0.15;

  for (const auto & variant : withScaleVariants(base)) {
    EXPECT_EQ(variant.p2, base.p2);
    EXPECT_EQ(variant.p4, base.p4);
    EXPECT_DOUBLE_EQ(variant.r_comm, base.r_comm);
    EXPECT_DOUBLE_EQ(variant.swap_threshold, base.swap_threshold);
    // Gorev alani da degismemeli (plan Bolum 1).
    EXPECT_DOUBLE_EQ(variant.sim.area_side, base.sim.area_side);
    EXPECT_EQ(variant.missionArea().cellCount(), base.missionArea().cellCount());
  }
}

TEST(ParameterSpace, DeneyKimlikleriYalnizcaNEkiyleAyrisir)
{
  ExperimentConfig base;
  base.p2 = swarm_bt_core::CoordinationArchitecture::kHierarchicalHybrid;
  base.p3 = swarm_bt_core::AllocationAlgorithm::kCbba;
  base.p4 = swarm_bt_core::BtArchitecture::kEventDriven;
  base.p5 = swarm_bt_core::CommunicationMechanisms::fromLetters("bc");
  base.p6 = swarm_bt_core::TriggerModel::kEventDriven;

  const auto variants = withScaleVariants(base);
  EXPECT_EQ(variants[0].experimentId(), "P2b_P3c_P4c_P5bc_P6c_N3");
  EXPECT_EQ(variants[1].experimentId(), "P2b_P3c_P4c_P5bc_P6c_N5");
  EXPECT_EQ(combinationId(variants[0]), combinationId(variants[1]));
  EXPECT_EQ(combinationId(variants[0]), "P2b_P3c_P4c_P5bc_P6c");
}

TEST(ParameterSpace, KombinasyonListesiCogaltilir)
{
  std::vector<ExperimentConfig> bases(3);
  bases[0].p4 = swarm_bt_core::BtArchitecture::kCentral;
  bases[1].p4 = swarm_bt_core::BtArchitecture::kDistributed;
  bases[2].p4 = swarm_bt_core::BtArchitecture::kEventDriven;

  const auto variants = withScaleVariants(bases);
  EXPECT_EQ(variants.size(), 6u);

  std::set<std::string> ids;
  for (const auto & variant : variants) {
    ids.insert(variant.experimentId());
  }
  EXPECT_EQ(ids.size(), 6u) << "kimlikler benzersiz degil";

  // Her kombinasyon tam olarak iki kez, farkli N ile gorunmeli.
  std::set<std::string> combinations;
  for (const auto & variant : variants) {
    combinations.insert(combinationId(variant));
  }
  EXPECT_EQ(combinations.size(), 3u);
}

TEST(ParameterSpace, GecersizTabanKombinasyonReddedilir)
{
  ExperimentConfig base;
  base.r_comm = -1.0;
  EXPECT_THROW(withScaleVariants(base), std::invalid_argument);
}
