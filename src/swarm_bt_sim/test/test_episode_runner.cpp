// Uctan uca koşu: kurulum, dongu, karsilasma islemesi ve metrik toplama.
#include <gtest/gtest.h>

#include <set>
#include <string>

#include <swarm_bt_core/experiment_config.hpp>

#include "swarm_bt_sim/episode_runner.hpp"

#ifndef SWARM_BT_XML_DIR
#error "SWARM_BT_XML_DIR tanimli degil (CMakeLists.txt'e bakin)"
#endif

namespace
{
/// Testler kurulu pakete degil kaynak agacindaki BT XML'lerine bakar.
swarm_bt_sim::EpisodeRunner makeRunner(
  const swarm_bt_core::ExperimentConfig & config, int seed)
{
  return swarm_bt_sim::EpisodeRunner(config, seed, SWARM_BT_XML_DIR);
}
}  // namespace

using swarm_bt_core::ExperimentConfig;
using swarm_bt_sim::EpisodeRunner;

TEST(EpisodeRunner, BaselineKoşusuKapsamayiTamamlar)
{
  ExperimentConfig config;
  auto runner = makeRunner(config, 0);
  const auto metrics = runner.run();

  EXPECT_TRUE(metrics.coverage_complete);
  EXPECT_GT(metrics.ticks, 0);
  EXPECT_GT(metrics.mission_time, 0.0);
  EXPECT_LT(metrics.mission_time, config.sim.time_limit);
  EXPECT_GT(metrics.total_distance, 0.0);
}

TEST(EpisodeRunner, TekrarlarArasindaGercekDegiskenlikVar)
{
  // Kalkis konumlari tohumdan turedigi icin koşular birbirinden farkli olmali;
  // aksi halde plan Bolum 5/Faz 1'in istedigi >=10 tekrar hicbir sey ortalamaz.
  ExperimentConfig config;
  std::set<double> times;
  int total_encounters = 0;
  for (int seed = 0; seed < 5; ++seed) {
    const auto metrics = makeRunner(config, seed).run();
    times.insert(metrics.mission_time);
    total_encounters += metrics.encounters;
    EXPECT_TRUE(metrics.coverage_complete) << "tohum " << seed;
  }
  EXPECT_GT(times.size(), 3u) << "koşular birbirinin ayni cikiyor";
  EXPECT_GT(total_encounters, 0) << "10 koşuda hic karsilasma olmadi";
}

TEST(EpisodeRunner, AyniTohumAyniMetrikleriUretir)
{
  ExperimentConfig config;
  const auto a = makeRunner(config, 5).run();
  const auto b = makeRunner(config, 5).run();

  EXPECT_DOUBLE_EQ(a.mission_time, b.mission_time);
  EXPECT_EQ(a.encounters, b.encounters);
  EXPECT_EQ(a.swaps, b.swaps);
  EXPECT_DOUBLE_EQ(a.total_distance, b.total_distance);
}

TEST(EpisodeRunner, BesDroneUcDroneDenHizliBitirirVeDahaCokKarsilasir)
{
  // Tekil tohumlar gurultulu; ortalama uzerinden bakilir.
  ExperimentConfig config;
  double time_three = 0.0;
  double time_five = 0.0;
  int encounters_three = 0;
  int encounters_five = 0;

  for (int seed = 0; seed < 5; ++seed) {
    config.n_agents = 3;
    const auto three = makeRunner(config, seed).run();
    config.n_agents = 5;
    const auto five = makeRunner(config, seed).run();
    time_three += three.mission_time;
    time_five += five.mission_time;
    encounters_three += three.encounters;
    encounters_five += five.encounters;
  }

  EXPECT_LT(time_five, time_three);
  // Plan Bolum 1'in dayanagi: alan sabitken N=5 daha yogun -> daha sik karsilasma.
  EXPECT_GT(encounters_five, encounters_three);
}

TEST(EpisodeRunner, ArizasizKoşudaDevralmaOlmaz)
{
  ExperimentConfig config;
  config.failure.enabled = false;
  const auto metrics = makeRunner(config, 0).run();

  EXPECT_EQ(metrics.orphan_transfers, 0);
  EXPECT_EQ(metrics.failed_agent, -1);
  EXPECT_TRUE(metrics.coverage_complete);
}

TEST(EpisodeRunner, ArizaTetiklenirVeAlanDevralinir)
{
  // Plan Bolum 2.3: arizalanan drone'un kalan alani devralinmali ve gorev
  // yine de tamamlanabilmeli.
  ExperimentConfig config;
  config.failure.enabled = true;
  config.failure.time = -1.0;    // gorev ortasi
  config.failure.agent_id = 1;   // ortadaki serit
  const auto metrics = makeRunner(config, 0).run();

  EXPECT_EQ(metrics.failed_agent, 1);
  EXPECT_GT(metrics.failure_time, 0.0);
  EXPECT_GT(metrics.orphan_transfers, 0);
  EXPECT_GT(metrics.assignment_stability, 0.0);
}

TEST(EpisodeRunner, ArizaGorevSuresiniUzatir)
{
  ExperimentConfig config;
  double healthy_total = 0.0;
  double degraded_total = 0.0;
  for (int seed = 0; seed < 5; ++seed) {
    healthy_total += makeRunner(config, seed).run().mission_time;

    auto with_failure = config;
    with_failure.failure.enabled = true;
    with_failure.failure.time = -1.0;
    with_failure.failure.agent_id = 1;
    degraded_total += makeRunner(with_failure, seed).run().mission_time;
  }
  EXPECT_GT(degraded_total, healthy_total);
}

TEST(EpisodeRunner, ChurnOraniSifirIleBirArasinda)
{
  // churn orani karsilasma BASINA tanimli: bosta kalan ajanin devralmasi
  // (idle_claims) paya girmemeli, yoksa oran 1'i asabilir.
  ExperimentConfig config;
  config.failure.enabled = true;
  config.failure.time = -1.0;
  for (int seed = 0; seed < 5; ++seed) {
    const auto metrics = makeRunner(config, seed).run();
    EXPECT_GE(metrics.churn_ratio, 0.0) << "tohum " << seed;
    EXPECT_LE(metrics.churn_ratio, 1.0) << "tohum " << seed;
    EXPECT_LE(metrics.churn_events, metrics.encounters) << "tohum " << seed;
  }
}

TEST(EpisodeRunner, GecersizKonfigurasyonReddedilir)
{
  ExperimentConfig config;
  config.n_agents = 0;
  EXPECT_THROW(makeRunner(config, 0), std::invalid_argument);
}

// --- P6 tetikleme modelleri (Bolum 3 / Bolum 4 OFAT ekseni) ---

namespace
{
swarm_bt_sim::EpisodeMetrics runWithTrigger(swarm_bt_core::TriggerModel model, int seed)
{
  ExperimentConfig config;
  config.p6 = model;
  config.failure.enabled = true;
  config.failure.time = -1.0;
  return makeRunner(config, seed).run();
}
}  // namespace

TEST(EpisodeRunner, P6TetiklemeModelleriKoordinasyonSikligiyleAyrisir)
{
  // P6b her tick menzildeki tum ciftleri isler -> P6c'den cok daha fazla
  // koordinasyon karari. Karsilasma SAYISI ise ikisinde de ayni kalmali:
  // "karsilasma sikligi" comm-range giris sayisidir, karar sayisi degil.
  int event_driven_coordination = 0;
  int every_tick_coordination = 0;
  int event_driven_encounters = 0;
  int every_tick_encounters = 0;

  for (int seed = 0; seed < 5; ++seed) {
    const auto event_driven = runWithTrigger(swarm_bt_core::TriggerModel::kEventDriven, seed);
    const auto every_tick = runWithTrigger(swarm_bt_core::TriggerModel::kEveryTick, seed);
    event_driven_coordination += event_driven.coordination_events;
    every_tick_coordination += every_tick.coordination_events;
    event_driven_encounters += event_driven.encounters;
    every_tick_encounters += every_tick.encounters;
  }

  EXPECT_GT(every_tick_coordination, event_driven_coordination);
  EXPECT_GT(event_driven_encounters, 0);
}

TEST(EpisodeRunner, P6cKoordinasyonKarariKarsilasmaBasinaBirKez)
{
  // Saf olay-tetiklemeli modelde her comm-range girisi tam olarak bir karar.
  for (int seed = 0; seed < 5; ++seed) {
    const auto metrics = runWithTrigger(swarm_bt_core::TriggerModel::kEventDriven, seed);
    EXPECT_EQ(metrics.coordination_events, metrics.encounters) << "tohum " << seed;
  }
}

TEST(EpisodeRunner, P6aPeriyodikYoklamaDahaAzKontrolYapar)
{
  // Yoklama maliyeti: P6a yalnizca poll_period'da bir bakar.
  for (int seed = 0; seed < 5; ++seed) {
    const auto periodic = runWithTrigger(swarm_bt_core::TriggerModel::kPeriodicPolling, seed);
    const auto every_tick = runWithTrigger(swarm_bt_core::TriggerModel::kEveryTick, seed);
    EXPECT_LT(periodic.detection_checks, every_tick.detection_checks) << "tohum " << seed;
  }
}

TEST(EpisodeRunner, P6aYoklamaArasindaGirenCiftleriKacirabilir)
{
  // Ucuz ama kayipli: iki yoklama arasinda girip cikan ciftler hic gorulmez.
  // Bu, OFAT taramasinda P6 ekseninin olctugu asil odunlesme.
  int periodic_total = 0;
  int every_tick_total = 0;
  for (int seed = 0; seed < 5; ++seed) {
    periodic_total +=
      runWithTrigger(swarm_bt_core::TriggerModel::kPeriodicPolling, seed).encounters;
    every_tick_total += runWithTrigger(swarm_bt_core::TriggerModel::kEveryTick, seed).encounters;
  }
  EXPECT_LE(periodic_total, every_tick_total);
}

TEST(EpisodeRunner, TumP6ModelleriKapsamayiTamamlar)
{
  for (const auto model : {
    swarm_bt_core::TriggerModel::kPeriodicPolling,
    swarm_bt_core::TriggerModel::kEveryTick,
    swarm_bt_core::TriggerModel::kEventDriven})
  {
    for (int seed = 0; seed < 5; ++seed) {
      EXPECT_TRUE(runWithTrigger(model, seed).coverage_complete)
        << "P6" << swarm_bt_core::toLetter(model) << " tohum " << seed;
    }
  }
}

// --- P5 iletisim mekanizmalari (Bolum 3 / Bolum 4 OFAT ekseni) ---

namespace
{
swarm_bt_sim::EpisodeMetrics runWithComms(const std::string & letters, int seed)
{
  ExperimentConfig config;
  config.p5 = swarm_bt_core::CommunicationMechanisms::fromLetters(letters);
  config.failure.enabled = true;
  config.failure.time = -1.0;
  return makeRunner(config, seed).run();
}

double meanMissionTime(const std::string & letters, int repetitions = 5)
{
  double total = 0.0;
  for (int seed = 0; seed < repetitions; ++seed) {
    total += runWithComms(letters, seed).mission_time;
  }
  return total / repetitions;
}
}  // namespace

TEST(EpisodeRunner, P5bStigmerjiAjanlarinBilgiKapsamasiniBelirler)
{
  // Stigmerji acikken her ajan taranmis TUM hucreleri bilir; kapaliyken
  // yalnizca kendi taradiklarini ve karsilasmalarda ogrendiklerini bilir.
  double with_stigmergy = 0.0;
  double without_stigmergy = 0.0;
  for (int seed = 0; seed < 5; ++seed) {
    with_stigmergy += runWithComms("abc", seed).known_coverage_ratio;
    without_stigmergy += runWithComms("ac", seed).known_coverage_ratio;
  }
  EXPECT_NEAR(with_stigmergy / 5.0, 1.0, 1e-9);
  EXPECT_LT(without_stigmergy / 5.0, 1.0);
}

TEST(EpisodeRunner, P5bStigmerjiKesinBolmedeGorevSuresiniDegistirmez)
{
  // BULGU (README V15): bolgeler kesismedigi icin mukerrer tarama hic
  // olusmuyor; stigmerjinin "baskasinin taradigini atla" faydasi bu modelde
  // BAGLAMIYOR. Etkisinin gorunecegi yer Bolum 2.2'nin ortak tarama
  // (joint_scan) dali; o dal negotiation alt-agaciyla birlikte gelecek.
  EXPECT_DOUBLE_EQ(meanMissionTime("abc"), meanMissionTime("ac"));
}

TEST(EpisodeRunner, P5aOlmadanTakasMuzakeresiYapilamaz)
{
  // Dogrudan mesaj yoksa ajanlar birbirinin kalan alanini ogrenemez.
  for (int seed = 0; seed < 5; ++seed) {
    const auto metrics = runWithComms("bc", seed);
    EXPECT_EQ(metrics.proposals, 0) << "tohum " << seed;
    EXPECT_EQ(metrics.swaps, 0) << "tohum " << seed;
  }
}

TEST(EpisodeRunner, P5aVarkenBilgiPaylasimiOlur)
{
  int with_direct = 0;
  int without_direct = 0;
  for (int seed = 0; seed < 5; ++seed) {
    with_direct += runWithComms("ab", seed).shared_cell_updates;
    without_direct += runWithComms("b", seed).shared_cell_updates;
  }
  EXPECT_GT(with_direct, without_direct);
  EXPECT_EQ(without_direct, 0);
}

TEST(EpisodeRunner, P5dKulakMisafiriUcuncuTarafaBilgiTasir)
{
  int with_eavesdrop = 0;
  for (int seed = 0; seed < 5; ++seed) {
    ExperimentConfig config;
    config.n_agents = 5;   // ucuncu tarafin menzilde olma sansi daha yuksek
    config.p5 = swarm_bt_core::CommunicationMechanisms::fromLetters("abd");
    config.failure.enabled = true;
    config.failure.time = -1.0;
    with_eavesdrop += makeRunner(config, seed).run().eavesdrop_events;
  }
  EXPECT_GT(with_eavesdrop, 0);

  for (int seed = 0; seed < 5; ++seed) {
    EXPECT_EQ(runWithComms("abc", seed).eavesdrop_events, 0) << "tohum " << seed;
  }
}

TEST(EpisodeRunner, P5cOlmadanBostaDevralmaYapilamaz)
{
  // Intent yayini yoksa sahipsiz alanin varligi suruye duyurulmaz; devralma
  // yalnizca karsilasma aninda mumkundur.
  for (int seed = 0; seed < 5; ++seed) {
    EXPECT_EQ(runWithComms("ab", seed).idle_claims, 0) << "tohum " << seed;
  }
  int with_intent = 0;
  for (int seed = 0; seed < 5; ++seed) {
    with_intent += runWithComms("abc", seed).idle_claims;
  }
  EXPECT_GT(with_intent, 0);
}

TEST(EpisodeRunner, IletisimsizKoşuCokmedenBiter)
{
  // P5none: hicbir mekanizma yok. Gorev tamamlanmayabilir ama koşu cokmemeli.
  for (int seed = 0; seed < 5; ++seed) {
    const auto metrics = runWithComms("none", seed);
    EXPECT_GT(metrics.ticks, 0) << "tohum " << seed;
    EXPECT_EQ(metrics.proposals, 0);
    EXPECT_EQ(metrics.shared_cell_updates, 0);
  }
}

// --- P2 koordinasyon mimarileri (Bolum 3 / Bolum 4 OFAT ekseni) ---

namespace
{
swarm_bt_sim::EpisodeMetrics runWithArchitecture(
  swarm_bt_core::CoordinationArchitecture architecture, int seed, int n = 3)
{
  ExperimentConfig config;
  config.n_agents = n;
  config.p2 = architecture;
  config.failure.enabled = true;
  config.failure.time = -1.0;
  config.failure.agent_id = 1;
  return makeRunner(config, seed).run();
}
}  // namespace

TEST(EpisodeRunner, TumP2MimarileriKapsamayiTamamlar)
{
  for (const auto architecture : {
    swarm_bt_core::CoordinationArchitecture::kCentral,
    swarm_bt_core::CoordinationArchitecture::kHierarchicalHybrid,
    swarm_bt_core::CoordinationArchitecture::kDistributed})
  {
    for (int seed = 0; seed < 5; ++seed) {
      EXPECT_TRUE(runWithArchitecture(architecture, seed).coverage_complete)
        << "P2" << swarm_bt_core::toLetter(architecture) << " tohum " << seed;
    }
  }
}

TEST(EpisodeRunner, P2aMerkeziMimariDevralmayiKarsilasmaBeklemedenYapar)
{
  // Merkezi mimarinin YAPISAL ozelligi: sahipsiz alanin devri bir karsilasmaya
  // bagli degildir (dagitikta oyle, README V12). Gorev suresi sirasi tohumdan
  // tohuma degisebilir; burada sinanan sey davranisin kendisi.
  //
  // Kurgu: r_comm cok kucuk -> hicbir karsilasma olmaz. Merkez yine de
  // arizali drone'un alanini dagitabilmeli.
  ExperimentConfig config;
  config.n_agents = 3;
  config.r_comm = 1.0;              // hicbir karsilasma olmasin
  config.sim.safety_radius = 0.5;
  // Rastgele kalkis iki drone'u 1 m icine duşurebilir; deterministik kalkista
  // ajanlar kendi seritlerinde, en az bir serit genisligi kadar uzakta baslar.
  config.sim.random_launch = false;
  config.p5.intent_broadcast = false;   // bosta devralma yolunu da kapat
  config.failure.enabled = true;
  config.failure.time = -1.0;
  config.failure.agent_id = 1;

  auto central = config;
  central.p2 = swarm_bt_core::CoordinationArchitecture::kCentral;
  auto distributed = config;
  distributed.p2 = swarm_bt_core::CoordinationArchitecture::kDistributed;

  int central_transfers = 0;
  int distributed_transfers = 0;
  int total_encounters = 0;
  for (int seed = 0; seed < 5; ++seed) {
    const auto with_center = makeRunner(central, seed).run();
    const auto without_center = makeRunner(distributed, seed).run();
    central_transfers += with_center.orphan_transfers;
    distributed_transfers += without_center.orphan_transfers;
    total_encounters += with_center.encounters + without_center.encounters;
  }

  EXPECT_EQ(total_encounters, 0) << "kurgu bozuldu: karsilasma olmamaliydi";
  EXPECT_GT(central_transfers, 0) << "merkez karsilasma olmadan devri yapamadi";
  EXPECT_EQ(distributed_transfers, 0)
    << "dagitik mimari karsilasma olmadan devir yapmamali";
}

TEST(EpisodeRunner, P2aMerkeziMimariDahaCokMesajUretir)
{
  // Merkezi mimaride her koordinasyon adiminda tum ajanlar merkeze rapor verir.
  for (int seed = 0; seed < 5; ++seed) {
    const auto central =
      runWithArchitecture(swarm_bt_core::CoordinationArchitecture::kCentral, seed, 5);
    const auto distributed =
      runWithArchitecture(swarm_bt_core::CoordinationArchitecture::kDistributed, seed, 5);
    EXPECT_GT(central.coordination_messages, distributed.coordination_messages)
      << "tohum " << seed;
  }
}

TEST(EpisodeRunner, P2bHiyerarsikMimarideGeciciLiderSecilir)
{
  int elections = 0;
  for (int seed = 0; seed < 5; ++seed) {
    elections += runWithArchitecture(
      swarm_bt_core::CoordinationArchitecture::kHierarchicalHybrid, seed, 5).leader_elections;
  }
  EXPECT_GT(elections, 0);

  // Dagitik ve merkezi mimarilerde lider secimi olmamali.
  for (int seed = 0; seed < 5; ++seed) {
    EXPECT_EQ(
      runWithArchitecture(swarm_bt_core::CoordinationArchitecture::kDistributed, seed)
      .leader_elections, 0);
    EXPECT_EQ(
      runWithArchitecture(swarm_bt_core::CoordinationArchitecture::kCentral, seed)
      .leader_elections, 0);
  }
}

TEST(EpisodeRunner, P2aMerkeziMimariDigerIkisindenAyrisir)
{
  double central = 0.0;
  double hierarchical = 0.0;
  double distributed = 0.0;
  for (int seed = 0; seed < 5; ++seed) {
    central +=
      runWithArchitecture(swarm_bt_core::CoordinationArchitecture::kCentral, seed).mission_time;
    hierarchical += runWithArchitecture(
      swarm_bt_core::CoordinationArchitecture::kHierarchicalHybrid, seed).mission_time;
    distributed += runWithArchitecture(
      swarm_bt_core::CoordinationArchitecture::kDistributed, seed).mission_time;
  }
  EXPECT_NE(central, distributed);
  EXPECT_NE(central, hierarchical);
}

TEST(EpisodeRunner, P2bHiyerarsikMimariUcuncuBirDavranisUretir)
{
  // BULGU (README V16, revize): BT karar katmani devreye girdikten sonra
  // hiyerarsik mimari dagitiktan AYRISIYOR. Lider kumeyi butun olarak gorup
  // is yukunu dogrudan yeniden dagitiyor; dagitik mimaride ayni is yalnizca
  // ikili muzakere yoluyla, karsilasma sirasina bagli olarak yapiliyor.
  double hierarchical_total = 0.0;
  double distributed_total = 0.0;
  for (int seed = 0; seed < 6; ++seed) {
    hierarchical_total += runWithArchitecture(
      swarm_bt_core::CoordinationArchitecture::kHierarchicalHybrid, seed, 5).mission_time;
    distributed_total += runWithArchitecture(
      swarm_bt_core::CoordinationArchitecture::kDistributed, seed, 5).mission_time;
  }
  EXPECT_NE(hierarchical_total, distributed_total);
  // Lider kumeyi bir butun olarak planladigi icin daha hizli bitirmeli.
  EXPECT_LT(hierarchical_total, distributed_total);
}
