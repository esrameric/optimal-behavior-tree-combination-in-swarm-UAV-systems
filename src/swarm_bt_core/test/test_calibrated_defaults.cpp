// Plan Bolum 10: r_comm ve esik_degeri icin baslangic degerleri.
//
// Bu test, kalibre edilmis degerlerin BELGEDEKI degerlerle ayni kalmasini
// garanti eder (experiments/kalibre_parametreler.md). Bir degeri degistirip
// belgeyi guncellemeyi unutmak mumkun olmasin diye buradalar: degerler
// koddan sessizce kayamaz.
#include <gtest/gtest.h>

#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/parameter_space.hpp"

using swarm_bt_core::ExperimentConfig;

TEST(CalibratedDefaults, RCommBolum1DeBelirlenenDeger)
{
  // 400 m alanin %15'i; planin onerdigi %10-20 bandinin ortasi.
  const ExperimentConfig config;
  EXPECT_DOUBLE_EQ(config.r_comm, 60.0);
  EXPECT_NEAR(config.r_comm / config.sim.area_side, 0.15, 1e-9);
}

TEST(CalibratedDefaults, EsikDegeriBolum5Faz0daRevizeEdilenDeger)
{
  // Planin onerdigi 0.30 calisma bandinin disinda; ilk kalibrasyondaki 0.20
  // BT modeliyle fazla yuksek cikti.
  const ExperimentConfig config;
  EXPECT_DOUBLE_EQ(config.swap_threshold, 0.10);
}

TEST(CalibratedDefaults, HisterezisVeHizSapmasiSifirDegil)
{
  // Ikisi de sifirlandiginda model dejenere oluyor (README V8, V9).
  const ExperimentConfig config;
  EXPECT_GT(config.encounter_hysteresis, 0.0);
  EXPECT_GT(config.sim.speed_jitter, 0.0);
  EXPECT_DOUBLE_EQ(config.encounter_hysteresis, 0.10);
  EXPECT_DOUBLE_EQ(config.sim.speed_jitter, 0.05);
}

TEST(CalibratedDefaults, OrtakTaramaVeIlgiNoktasiDegerleri)
{
  const ExperimentConfig config;
  EXPECT_DOUBLE_EQ(config.sim.joint_scan_threshold, 0.25);
  EXPECT_EQ(config.sim.interest_points, 12);
}

TEST(CalibratedDefaults, GorevAlaniBelgedekiDegerlerde)
{
  const ExperimentConfig config;
  EXPECT_DOUBLE_EQ(config.sim.area_side, 400.0);
  EXPECT_DOUBLE_EQ(config.sim.cell_size, 20.0);
  EXPECT_EQ(config.missionArea().cellCount(), 400);
}

TEST(CalibratedDefaults, GuvenlikYaricapiCarpismaEsigi)
{
  const ExperimentConfig config;
  EXPECT_DOUBLE_EQ(config.sim.safety_radius, 5.0);
  EXPECT_LT(config.sim.safety_radius, config.r_comm)
    << "guvenlik yaricapi iletisim menzilinden kucuk olmali";
}

TEST(CalibratedDefaults, BaselineAyniKalibreDegerleriKullanir)
{
  // Baseline ile varsayilanlar ayrilirsa, "yalnizca farkli parametreyi yaz"
  // mantigi bozulur (Bolum 4).
  const auto baseline = swarm_bt_core::baselineConfig();
  const ExperimentConfig defaults;
  EXPECT_DOUBLE_EQ(baseline.r_comm, defaults.r_comm);
  EXPECT_DOUBLE_EQ(baseline.swap_threshold, defaults.swap_threshold);
  EXPECT_DOUBLE_EQ(baseline.sim.area_side, defaults.sim.area_side);
  EXPECT_EQ(baseline.experimentId(), defaults.experimentId());
}

TEST(CalibratedDefaults, VarsayilanKonfigurasyonGecerli)
{
  EXPECT_NO_THROW(ExperimentConfig{}.validate());
}
