// Plan Bolum 3: N in {3,5} olcek degiskeni ve kombinasyon x olcek cogaltmasi.
#include <gtest/gtest.h>

#include <algorithm>
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

// --- Baseline ve OFAT eksenleri (Bolum 4) ---

TEST(ParameterSpace, BaselinePlandakiKombinasyon)
{
  // Plan Bolum 4 onerisi: P2c + P3c + P4b + P5abc + P6c
  const auto baseline = swarm_bt_core::baselineConfig();
  EXPECT_EQ(baseline.experimentId(), "P2c_P3c_P4b_P5abc_P6c_N3");
}

TEST(ParameterSpace, VarsayilanKonfigurasyonBaselineIleAyni)
{
  // Bir deney dosyasi yalnizca farkli olan parametreyi yazdiginda geri kalani
  // baseline olmali; bu ancak varsayilanlar baseline'a esitse dogru olur.
  EXPECT_EQ(ExperimentConfig{}.experimentId(), swarm_bt_core::baselineConfig().experimentId());
}

TEST(ParameterSpace, BaselineKalibreEdilmisDegerleriKullanir)
{
  const auto baseline = swarm_bt_core::baselineConfig();
  EXPECT_DOUBLE_EQ(baseline.r_comm, 60.0);          // Bolum 1
  EXPECT_DOUBLE_EQ(baseline.swap_threshold, 0.10);  // Bolum 5/Faz 0 (yeniden kalibre)
}

TEST(ParameterSpace, OfatEksenleriP4uIcermez)
{
  // P4 agacin YAPISINI degistirir; config ekseni degil, ayri XML dosyalari.
  for (const auto & axis : swarm_bt_core::ofatAxes()) {
    EXPECT_NE(axis.name, "P4");
  }
  EXPECT_EQ(swarm_bt_core::btArchitectureAxis().name, "P4");
  EXPECT_EQ(swarm_bt_core::btArchitectureAxis().options.size(), 3u);
}

TEST(ParameterSpace, OfatVaryantlariBaselineIleBaslar)
{
  const auto baseline = swarm_bt_core::baselineConfig();
  const auto variants = swarm_bt_core::ofatVariants(baseline);

  ASSERT_FALSE(variants.empty());
  EXPECT_EQ(variants.front().experimentId(), baseline.experimentId());
}

TEST(ParameterSpace, HerOfatVaryantiBaselineDenTekParametreFarkli)
{
  const auto baseline = swarm_bt_core::baselineConfig();
  for (const auto & variant : swarm_bt_core::ofatVariants(baseline)) {
    int differences = 0;
    differences += (variant.p2 != baseline.p2) ? 1 : 0;
    differences += (variant.p3 != baseline.p3) ? 1 : 0;
    differences += (variant.p4 != baseline.p4) ? 1 : 0;
    differences += (variant.p5.toLetters() != baseline.p5.toLetters()) ? 1 : 0;
    differences += (variant.p6 != baseline.p6) ? 1 : 0;
    EXPECT_LE(differences, 1) << variant.experimentId() << " birden fazla parametre degistirmis";
  }
}

TEST(ParameterSpace, OfatVaryantlariBenzersizVeBaselineTekrarlanmaz)
{
  const auto variants = swarm_bt_core::ofatVariants(swarm_bt_core::baselineConfig());
  std::set<std::string> ids;
  for (const auto & variant : variants) {
    EXPECT_TRUE(ids.insert(variant.experimentId()).second)
      << variant.experimentId() << " iki kez uretilmis";
  }
  // 4 eksen (2+2+7 farkli P5+2) + P4'ten 2 = 15 varyant + baseline
  EXPECT_EQ(variants.size(), 16u);
}

TEST(ParameterSpace, OfatVaryantlariOlcekleCogaltilinca32Koşu)
{
  // Plan Bolum 4: "BU ASAMADA HER KOMBINASYON HEM N=3 HEM N=5 ILE KOŞULUR"
  const auto variants = swarm_bt_core::ofatVariants(swarm_bt_core::baselineConfig());
  const auto with_scales = withScaleVariants(variants);
  EXPECT_EQ(with_scales.size(), variants.size() * 2);
  EXPECT_EQ(with_scales.size(), 32u);
}

// --- Bolum 9 istege bagli genisletmeleri ---

TEST(ParameterSpace, GenisletilmisOlcekKumesiAnaKumeyiIcerir)
{
  // Ucuncu bir N degeri eklemek, mevcut iki degeri KORUYARAK yapilmali;
  // aksi halde onceki sonuclarla karsilastirilamaz.
  const auto & extended = swarm_bt_core::extendedScaleValues();
  for (const int n : scaleValues()) {
    EXPECT_NE(std::find(extended.begin(), extended.end(), n), extended.end())
      << "N=" << n << " genisletilmis kumede yok";
  }
  EXPECT_GT(extended.size(), scaleValues().size());
}

TEST(ParameterSpace, OrantiliAlanDroneBasinaAlaniSabitTutar)
{
  // Plan Bolum 9: "drone basina duşen alan sabit kalacak sekilde".
  const auto base = swarm_bt_core::baselineConfig();
  const double reference_per_agent =
    swarm_bt_core::withProportionalArea(base, 3, 3).cellsPerAgent();

  for (const int n : swarm_bt_core::extendedScaleValues()) {
    const auto scaled = swarm_bt_core::withProportionalArea(base, n, 3);
    // Hucre sayisi tam sayiya yuvarlandigi icin kucuk sapma kacinilmaz.
    EXPECT_NEAR(scaled.cellsPerAgent(), reference_per_agent, reference_per_agent * 0.12)
      << "N=" << n << " drone basina alan sabit kalmadi";
  }
}

TEST(ParameterSpace, OrantiliAlanKenariKokNIleBuyur)
{
  const auto base = swarm_bt_core::baselineConfig();
  const auto scaled = swarm_bt_core::withProportionalArea(base, 12, 3);
  // sqrt(12/3) = 2 -> kenar iki katina cikmali
  EXPECT_NEAR(scaled.sim.area_side, base.sim.area_side * 2.0, 1e-9);
}

TEST(ParameterSpace, OrantiliAlanRCommuAyniOrandaOlcekler)
{
  // Menzil olceklenmezse goreli olarak kuculur ve karsilasma sikligi alan
  // degisiminden etkilenir; kontrol deneyinin amaci tam olarak bunu onlemek.
  const auto base = swarm_bt_core::baselineConfig();
  const auto scaled = swarm_bt_core::withProportionalArea(base, 12, 3);
  EXPECT_NEAR(scaled.r_comm / scaled.sim.area_side, base.r_comm / base.sim.area_side, 1e-9);
}

TEST(ParameterSpace, OrantiliAlanGizliNBagimliligiEklemez)
{
  // Bolum 1 invaryanti korunmali: missionArea() N'e bakmamali. Alan yalnizca
  // sim.area_side uzerinden degismeli.
  const auto base = swarm_bt_core::baselineConfig();
  const auto scaled = swarm_bt_core::withProportionalArea(base, 10, 3);

  auto probe = scaled;
  probe.n_agents = 3;
  EXPECT_EQ(probe.missionArea().cellCount(), scaled.missionArea().cellCount())
    << "missionArea() N'e bagimli hale gelmis";
}

TEST(ParameterSpace, OrantiliAlanGecersizGirdiyiReddeder)
{
  const auto base = swarm_bt_core::baselineConfig();
  EXPECT_THROW(swarm_bt_core::withProportionalArea(base, 0, 3), std::invalid_argument);
  EXPECT_THROW(swarm_bt_core::withProportionalArea(base, 3, 0), std::invalid_argument);
}
