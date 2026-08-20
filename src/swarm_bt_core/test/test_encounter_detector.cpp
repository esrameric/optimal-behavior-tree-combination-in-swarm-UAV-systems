#include <gtest/gtest.h>

#include <vector>

#include "swarm_bt_core/encounter_detector.hpp"

using swarm_bt_core::EncounterDetector;
using swarm_bt_core::MissionArea;
using swarm_bt_core::SwarmState;
using swarm_bt_core::Vec2;

namespace
{
SwarmState twoAgentsAt(double x_a, double x_b)
{
  SwarmState state(MissionArea(400.0, 400.0, 20.0), 2, 0.0);
  state.agent(0).position = Vec2{x_a, 0.0};
  state.agent(1).position = Vec2{x_b, 0.0};
  return state;
}
}  // namespace

TEST(EncounterDetector, GecersizMenzilReddedilir)
{
  EXPECT_THROW(EncounterDetector(0.0), std::invalid_argument);
  EXPECT_THROW(EncounterDetector(-5.0), std::invalid_argument);
}

TEST(EncounterDetector, MenzilDisindaOlayYok)
{
  EncounterDetector detector(50.0);
  auto state = twoAgentsAt(0.0, 100.0);
  EXPECT_TRUE(detector.update(state).empty());
  EXPECT_EQ(detector.totalEncounters(), 0);
}

TEST(EncounterDetector, MenzileGirisTekOlayUretir)
{
  EncounterDetector detector(50.0);
  auto state = twoAgentsAt(0.0, 40.0);

  const auto events = detector.update(state);
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].agent_a, 0);
  EXPECT_EQ(events[0].agent_b, 1);
  EXPECT_DOUBLE_EQ(events[0].distance, 40.0);
  EXPECT_EQ(detector.totalEncounters(), 1);
}

TEST(EncounterDetector, MenzilIcindeKalmakTekrarOlayUretmez)
{
  // Yukselen kenar mantigi: "karsilasma sikligi" metrigi giris SAYISINI olcer,
  // menzil icinde gecirilen tick sayisini degil.
  EncounterDetector detector(50.0);
  auto state = twoAgentsAt(0.0, 40.0);

  EXPECT_EQ(detector.update(state).size(), 1u);
  for (int i = 0; i < 10; ++i) {
    EXPECT_TRUE(detector.update(state).empty()) << "tick " << i;
  }
  EXPECT_EQ(detector.totalEncounters(), 1);
  EXPECT_EQ(detector.pairsInRange(), 1u);
}

TEST(EncounterDetector, CikipTekrarGirmeYeniOlayUretir)
{
  EncounterDetector detector(50.0);
  auto state = twoAgentsAt(0.0, 40.0);
  EXPECT_EQ(detector.update(state).size(), 1u);

  state.agent(1).position = Vec2{200.0, 0.0};   // menzilden cik
  EXPECT_TRUE(detector.update(state).empty());
  EXPECT_EQ(detector.pairsInRange(), 0u);

  state.agent(1).position = Vec2{30.0, 0.0};    // tekrar gir
  EXPECT_EQ(detector.update(state).size(), 1u);
  EXPECT_EQ(detector.totalEncounters(), 2);
}

TEST(EncounterDetector, MenzilSinirindaDahilKabulEdilir)
{
  EncounterDetector detector(50.0);
  auto state = twoAgentsAt(0.0, 50.0);
  EXPECT_EQ(detector.update(state).size(), 1u);
}

TEST(EncounterDetector, ArizaliAjanKarsilasmaUretmez)
{
  // Bolum 2.3 surpriz olayi: arizalanan drone iletisim grafigi disina cikar.
  EncounterDetector detector(50.0);
  auto state = twoAgentsAt(0.0, 40.0);
  state.agent(1).alive = false;
  EXPECT_TRUE(detector.update(state).empty());
  EXPECT_EQ(detector.totalEncounters(), 0);
}

TEST(EncounterDetector, AyniAndaCokluCiftTespitEdilir)
{
  SwarmState state(MissionArea(400.0, 400.0, 20.0), 3, 0.0);
  state.agent(0).position = Vec2{0.0, 0.0};
  state.agent(1).position = Vec2{10.0, 0.0};
  state.agent(2).position = Vec2{20.0, 0.0};

  EncounterDetector detector(50.0);
  EXPECT_EQ(detector.update(state).size(), 3u);  // (0,1), (0,2), (1,2)
  EXPECT_EQ(detector.totalEncounters(), 3);
}

TEST(EncounterDetector, SifirlamaSayaclariTemizler)
{
  EncounterDetector detector(50.0);
  auto state = twoAgentsAt(0.0, 40.0);
  detector.update(state);
  detector.reset();
  EXPECT_EQ(detector.totalEncounters(), 0);
  EXPECT_EQ(detector.pairsInRange(), 0u);
  EXPECT_EQ(detector.update(state).size(), 1u);
}
