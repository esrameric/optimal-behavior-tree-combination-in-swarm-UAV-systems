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

// --- Histerezis (Bolum 1 kalibrasyonunda ortaya cikan chattering sorunu) ---

TEST(EncounterDetector, NegatifHisterezisReddedilir)
{
  EXPECT_THROW(EncounterDetector(50.0, -0.1), std::invalid_argument);
}

TEST(EncounterDetector, CikisEsigiGirisEsigindenBuyuk)
{
  const EncounterDetector detector(50.0, 0.2);
  EXPECT_DOUBLE_EQ(detector.commRange(), 50.0);
  EXPECT_DOUBLE_EQ(detector.exitRange(), 60.0);
}

TEST(EncounterDetector, HisterezisBandindaKalmakCikisSaymaz)
{
  EncounterDetector detector(50.0, 0.2);   // giris 50, cikis 60
  auto state = twoAgentsAt(0.0, 45.0);
  EXPECT_EQ(detector.update(state).size(), 1u);

  // Giris esiginin uzerine cik ama cikis esigini asma -> hala menzilde.
  state.agent(1).position = Vec2{55.0, 0.0};
  EXPECT_TRUE(detector.update(state).empty());
  EXPECT_EQ(detector.pairsInRange(), 1u);

  // Geri gel -> yeni olay URETILMEMELI (ayni karsilasmanin devami).
  state.agent(1).position = Vec2{45.0, 0.0};
  EXPECT_TRUE(detector.update(state).empty());
  EXPECT_EQ(detector.totalEncounters(), 1);
}

TEST(EncounterDetector, CikisEsigiAsilincaYeniKarsilasmaMumkun)
{
  EncounterDetector detector(50.0, 0.2);
  auto state = twoAgentsAt(0.0, 45.0);
  EXPECT_EQ(detector.update(state).size(), 1u);

  state.agent(1).position = Vec2{61.0, 0.0};   // cikis esigini (60) asti
  EXPECT_TRUE(detector.update(state).empty());
  EXPECT_EQ(detector.pairsInRange(), 0u);

  state.agent(1).position = Vec2{45.0, 0.0};
  EXPECT_EQ(detector.update(state).size(), 1u);
  EXPECT_EQ(detector.totalEncounters(), 2);
}

TEST(EncounterDetector, EsigeTegetSalinimTekKarsilasmaSayilir)
{
  // Bolum 1'de olculen gercek durum: bicerdover deseninde iki drone bitisik
  // sutunlarda paralel supururken mesafe tam esikte salinir. Histerezissiz
  // her salinim yeni bir "karsilasma" uretiyordu.
  EncounterDetector with_hysteresis(20.0, 0.1);
  EncounterDetector without_hysteresis(20.0, 0.0);
  auto state = twoAgentsAt(0.0, 20.0);

  for (int i = 0; i < 50; ++i) {
    // Mesafe 20.00 <-> 20.02 arasinda salinsin (hiz sapmasinin yarattigi etki).
    state.agent(1).position = Vec2{20.0, (i % 2 == 0) ? 0.0 : 1.0};
    with_hysteresis.update(state);
    without_hysteresis.update(state);
  }

  EXPECT_EQ(with_hysteresis.totalEncounters(), 1);
  EXPECT_GT(without_hysteresis.totalEncounters(), 10);
}
