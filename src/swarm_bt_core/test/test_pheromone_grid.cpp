#include <gtest/gtest.h>

#include <vector>

#include "swarm_bt_core/pheromone_grid.hpp"

using swarm_bt_core::PheromoneGrid;

TEST(PheromoneGrid, GecersizParametrelerReddedilir)
{
  EXPECT_THROW(PheromoneGrid(0, 0.1), std::invalid_argument);
  EXPECT_THROW(PheromoneGrid(10, -0.1), std::invalid_argument);
  EXPECT_THROW(PheromoneGrid(10, 1.5), std::invalid_argument);
}

TEST(PheromoneGrid, BirakmaBirikimli)
{
  PheromoneGrid grid(4, 0.0);
  grid.deposit(1, 0.5);
  grid.deposit(1, 0.25);
  EXPECT_DOUBLE_EQ(grid.at(1), 0.75);
  EXPECT_DOUBLE_EQ(grid.total(), 0.75);
  EXPECT_DOUBLE_EQ(grid.maxValue(), 0.75);
}

TEST(PheromoneGrid, SonumlemeUsselAzalir)
{
  PheromoneGrid grid(2, 0.1);
  grid.set(0, 1.0);
  grid.decay();
  EXPECT_NEAR(grid.at(0), 0.9, 1e-12);
  grid.decay();
  EXPECT_NEAR(grid.at(0), 0.81, 1e-12);
}

TEST(PheromoneGrid, SonumlemesizGridDegismez)
{
  // "Ziyaret edildi" katmani kalici iz olmali: decay_rate = 0.
  PheromoneGrid grid(2, 0.0);
  grid.set(0, 1.0);
  for (int i = 0; i < 100; ++i) {
    grid.decay();
  }
  EXPECT_DOUBLE_EQ(grid.at(0), 1.0);
}

TEST(PheromoneGrid, CokSonumlenenDegerTamSifirlanir)
{
  // Kayan nokta artiklari "yuksek feromon" kosulunu kirletmemeli.
  PheromoneGrid grid(1, 0.5);
  grid.set(0, 1.0);
  for (int i = 0; i < 200; ++i) {
    grid.decay();
  }
  EXPECT_DOUBLE_EQ(grid.at(0), 0.0);
}

TEST(PheromoneGrid, KumeOrtalamasi)
{
  PheromoneGrid grid(4, 0.0);
  grid.set(0, 1.0);
  grid.set(1, 3.0);
  EXPECT_DOUBLE_EQ(grid.meanOver({0, 1}), 2.0);
  EXPECT_DOUBLE_EQ(grid.meanOver({2, 3}), 0.0);
  EXPECT_DOUBLE_EQ(grid.meanOver({}), 0.0);
}

TEST(PheromoneGrid, SinirDisiErisimReddedilir)
{
  PheromoneGrid grid(2, 0.0);
  EXPECT_THROW(grid.at(-1), std::out_of_range);
  EXPECT_THROW(grid.at(2), std::out_of_range);
  EXPECT_THROW(grid.deposit(5, 1.0), std::out_of_range);
  EXPECT_THROW(grid.set(5, 1.0), std::out_of_range);
}

TEST(PheromoneGrid, SifirlamaTumHucreleriTemizler)
{
  PheromoneGrid grid(3, 0.0);
  grid.set(0, 1.0);
  grid.set(2, 2.0);
  grid.reset();
  EXPECT_DOUBLE_EQ(grid.total(), 0.0);
}
