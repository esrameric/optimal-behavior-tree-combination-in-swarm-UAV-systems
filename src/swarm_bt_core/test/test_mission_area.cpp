#include <gtest/gtest.h>

#include <set>
#include <vector>

#include "swarm_bt_core/mission_area.hpp"

using swarm_bt_core::MissionArea;
using swarm_bt_core::Vec2;

TEST(MissionArea, GridBoyutlariHucreBoyutundanTurer)
{
  const MissionArea area(400.0, 400.0, 20.0);
  EXPECT_EQ(area.cols(), 20);
  EXPECT_EQ(area.rows(), 20);
  EXPECT_EQ(area.cellCount(), 400);
  EXPECT_DOUBLE_EQ(area.referenceSide(), 400.0);
}

TEST(MissionArea, GecersizParametrelerReddedilir)
{
  EXPECT_THROW(MissionArea(0.0, 400.0, 20.0), std::invalid_argument);
  EXPECT_THROW(MissionArea(400.0, 400.0, 0.0), std::invalid_argument);
  EXPECT_THROW(MissionArea(10.0, 10.0, 20.0), std::invalid_argument);
}

TEST(MissionArea, HucreMerkeziVeKonumdanHucreTutarli)
{
  const MissionArea area(400.0, 400.0, 20.0);
  for (int cell_id = 0; cell_id < area.cellCount(); ++cell_id) {
    const Vec2 center = area.cellCenter(cell_id);
    EXPECT_EQ(area.cellAt(center), cell_id) << "hucre " << cell_id;
  }
}

TEST(MissionArea, AlanDisiKonumGecersizHucreVerir)
{
  const MissionArea area(400.0, 400.0, 20.0);
  EXPECT_EQ(area.cellAt(Vec2{-1.0, 10.0}), -1);
  EXPECT_EQ(area.cellAt(Vec2{10.0, 401.0}), -1);
  EXPECT_FALSE(area.contains(Vec2{500.0, 10.0}));
  EXPECT_THROW(area.cellCenter(area.cellCount()), std::out_of_range);
}

TEST(MissionArea, BoustrophedonSirasiSutunSutunVeYonDegistirerek)
{
  // 3x3 grid: 0. sutun asagidan yukari, 1. sutun yukaridan asagi, 2. sutun tekrar yukari.
  const MissionArea area(30.0, 30.0, 10.0);
  std::vector<int> all;
  for (int i = 0; i < area.cellCount(); ++i) {
    all.push_back(i);
  }
  const auto order = area.boustrophedonOrder(all);

  const std::vector<int> expected = {
    area.cellId(0, 0), area.cellId(0, 1), area.cellId(0, 2),
    area.cellId(1, 2), area.cellId(1, 1), area.cellId(1, 0),
    area.cellId(2, 0), area.cellId(2, 1), area.cellId(2, 2)};
  EXPECT_EQ(order, expected);
}

TEST(MissionArea, BoustrophedonArdisikHucrelerKomsuKalir)
{
  // Bicerdover deseninin degeri: ardisik waypoint'ler arasi mesafe kucuk kalmali.
  const MissionArea area(200.0, 200.0, 20.0);
  std::vector<int> strip;
  for (int col = 0; col < 3; ++col) {
    for (int row = 0; row < area.rows(); ++row) {
      strip.push_back(area.cellId(col, row));
    }
  }
  const auto order = area.boustrophedonOrder(strip);
  for (std::size_t i = 1; i < order.size(); ++i) {
    const double d = swarm_bt_core::distance(
      area.cellCenter(order[i - 1]), area.cellCenter(order[i]));
    EXPECT_LE(d, area.cellSize() * 1.01) << "adim " << i;
  }
}
