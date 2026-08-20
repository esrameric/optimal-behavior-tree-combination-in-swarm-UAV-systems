// bringup/config altindaki deney YAML dosyalarinin gecerliligini dogrular.
// Sablon bozulursa ya da bir deney dosyasi hatali yazilirsa burada yakalanir.
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <swarm_bt_core/experiment_config.hpp>

#ifndef SWARM_BT_CONFIG_DIR
#error "SWARM_BT_CONFIG_DIR tanimli degil (CMakeLists.txt'e bakin)"
#endif

namespace
{
std::filesystem::path configDir()
{
  return std::filesystem::path(SWARM_BT_CONFIG_DIR);
}
}  // namespace

TEST(ConfigFiles, SablonDosyasiMevcut)
{
  EXPECT_TRUE(std::filesystem::exists(configDir() / "experiment_template.yaml"))
    << "aranan dizin: " << configDir();
}

TEST(ConfigFiles, SablonBaselineKombinasyonunuTanimlar)
{
  // Plan Bolum 4 onerilen baseline: P2c + P3c + P4b + P5abc + P6c, N=3.
  const auto config = swarm_bt_core::ExperimentConfig::fromYamlFile(
    (configDir() / "experiment_template.yaml").string());

  EXPECT_EQ(config.experimentId(), "P2c_P3c_P4b_P5abc_P6c_N3");
  EXPECT_EQ(config.n_agents, 3);
  EXPECT_DOUBLE_EQ(config.sim.area_side, 400.0);
  EXPECT_DOUBLE_EQ(config.swap_threshold, 0.20);
  EXPECT_EQ(config.repetitions, 10);
}

TEST(ConfigFiles, SablondakiDeneyKimligiAlaniTuretilenIleAyni)
{
  // YAML'daki okunabilirlik icin yazilan deney_id alani, parametrelerden
  // turetilen kimlikle tutarli olmali; aksi halde dosya adlari yaniltir.
  const auto path = (configDir() / "experiment_template.yaml").string();
  const auto config = swarm_bt_core::ExperimentConfig::fromYamlFile(path);

  std::ifstream file(path);
  ASSERT_TRUE(file.good());
  std::string line;
  std::string declared_id;
  while (std::getline(file, line)) {
    const std::string key = "deney_id:";
    if (line.rfind(key, 0) == 0) {
      declared_id = line.substr(key.size());
      declared_id.erase(0, declared_id.find_first_not_of(" \t"));
      break;
    }
  }
  EXPECT_EQ(declared_id, config.experimentId());
}

TEST(ConfigFiles, TumDeneyDosyalariGecerli)
{
  // Uretilmis experiment_*.yaml dosyalarinin hepsi ayristirilabilmeli.
  int checked = 0;
  for (const auto & entry : std::filesystem::directory_iterator(configDir())) {
    const auto name = entry.path().filename().string();
    if (name.rfind("experiment_", 0) != 0 || entry.path().extension() != ".yaml") {
      continue;
    }
    EXPECT_NO_THROW(
      swarm_bt_core::ExperimentConfig::fromYamlFile(entry.path().string())) << name;
    ++checked;
  }
  EXPECT_GT(checked, 0) << "hicbir experiment_*.yaml bulunamadi";
}
