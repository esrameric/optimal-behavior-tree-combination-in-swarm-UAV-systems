#ifndef SWARM_BT_SIM__EPISODE_RUNNER_HPP_
#define SWARM_BT_SIM__EPISODE_RUNNER_HPP_

#include <memory>
#include <utility>
#include <vector>

#include <swarm_bt_core/area_swap.hpp>
#include <swarm_bt_core/encounter_detector.hpp>
#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_core/failure_injector.hpp>
#include <swarm_bt_core/swarm_state.hpp>

#include "swarm_bt_sim/kinematic_sim.hpp"

namespace swarm_bt_sim
{

/// Tek bir koşunun sonunda toplanan olcumler (plan Bolum 6 ve Bolum 8).
struct EpisodeMetrics
{
  // --- temel metrikler ---
  double mission_time{0.0};        ///< gorev tamamlama suresi [s]
  int ticks{0};                    ///< tick maliyeti
  bool coverage_complete{false};
  double total_distance{0.0};      ///< tum ajanlarin kat ettigi toplam mesafe [m]

  // --- bu calismaya ozgu metrikler (Bolum 6) ---
  int encounters{0};               ///< karsilasma sikligi: comm-range giris sayisi
  /// Koordinasyon karari kac kez degerlendirildi (P6'ya gore degisir).
  /// P6c'de karsilasma sayisina esittir; P6b'de cok daha buyuktur.
  int coordination_events{0};
  /// Ikili mesafe kontrolunun kac tick'te calistigi.
  ///
  /// P6a'da yoklama araligina gore seyrektir; P6b ve P6c'de her tick calisir.
  /// P6c ile P6b arasindaki asil fark burada degil coordination_events'tedir:
  /// P6c yalnizca comm-range GIRISINDE karar verir, P6b menzilde kalindigi
  /// surece her tick yeniden degerlendirir. (Gercek bir sistemde P6c'nin giris
  /// tespiti telsiz katmaninda pasif olurdu; simulatorde ayni mesafe hesabi
  /// kullanildigi icin bu sayac ikisinde de ayni cikar.)
  int detection_checks{0};
  int proposals{0};                ///< dengesizlik esigini asip teklif kurulan karsilasma
  int swaps{0};                    ///< kabul edilip uygulanan takas sayisi
  int orphan_transfers{0};         ///< arizadan devralinan hucre sayisi
  /// Gercek bir atama degisikligine (takas ya da devralma) yol acan karsilasma
  /// sayisi. Bolum 6'daki churn oraninin payi budur.
  int churn_events{0};
  /// Karsilasma olmadan, bosta kalan ajanin sahipsiz alani ustlenmesi sayisi.
  int idle_claims{0};
  /// Iletisim yuku: karsilasmalarda karsi taraftan ogrenilen yeni hucre bilgisi
  /// toplami (P5a dogrudan mesaj + P5d kulak misafiri uzerinden).
  int shared_cell_updates{0};
  /// Kulak misafiri (P5d) ile bilgi alan ucuncu taraf sayisi.
  int eavesdrop_events{0};
  /// P2b'de kac kez gecici lider secildi (kume boyutu >= 2).
  int leader_elections{0};
  /// P2b'de olusan UC VE DAHA FAZLA uyeli kume sayisi. Hiyerarsik mimarinin
  /// dagitiktan ayristigi tek durum budur: ikili kumede lider, ikili
  /// pazarligin verecegi karari verir.
  int multi_agent_clusters{0};
  /// Iletisim yuku: koordinasyon icin gonderilen mesaj sayisi. P2a'da her
  /// koordinasyon adiminda tum ajanlar merkeze rapor verir (N mesaj); P2b'de
  /// kume buyuklugu kadar; P2c'de ikili basina 2.
  int coordination_messages{0};
  /// Koşu sonunda, ajanlarin gercekte taranmis hucrelerin ne kadarini BILDIGI
  /// (ajan basina ortalama oran). Stigmerji (P5b) acikken 1.0'dir; kapaliyken
  /// ajan yalnizca kendi taradiklarini ve karsilasmalarda ogrendiklerini bilir.
  double known_coverage_ratio{0.0};
  double assignment_stability{0.0};  ///< atama kararliligi: ajan basina atama degisikligi
  double churn_ratio{0.0};         ///< gercek degisiklige yol acan karsilasma / toplam
  double coverage_imbalance{0.0};  ///< koşu sonunda kalan alan std sapmasi

  // --- surpriz olay (Bolum 2.3) ---
  int failed_agent{-1};
  double failure_time{-1.0};
};

/// Tek bir deney koşusunu bastan sona calistirir.
///
/// Kurulum, dongu ve metrik toplama tek yerde toplanmistir; kalibrasyon
/// araclari, OFAT taramasi (Bolum 4) ve tekil koşu calistiricisi ayni kodu
/// kullanir, boylece olculen seyin tanimi her yerde aynidir.
///
/// NOT: Karsilasma anindaki karar su an dogrudan mekanizma cagrilariyla
/// yurutuluyor (devralma + takas degerlendirmesi). Bolum 5/Faz 0'da bu adim
/// negotiation BT alt-agacinin tiklenmesiyle degistirilecek; mekanizma
/// siniflari (AreaSwapNegotiator) BT dugumlerinin arkasinda ayni kalacak.
class EpisodeRunner
{
public:
  EpisodeRunner(const swarm_bt_core::ExperimentConfig & config, int seed);

  /// Koşuyu bitene kadar calistirir ve metrikleri dondurur.
  EpisodeMetrics run();

  const swarm_bt_core::SwarmState & state() const {return state_;}
  const swarm_bt_core::ExperimentConfig & config() const {return config_;}

private:
  /// Bir karsilasmayi isler: once arizadan kalan sahipsiz alan paylasilir,
  /// sonra dengesizlik esigi asilmissa takas degerlendirilir.
  void handleEncounter(int agent_a, int agent_b);

  /// P6 tetikleme modeline gore, bu tick'te hangi ciftlerin islenecegini belirler.
  void triggerCoordination();

  /// P5d kulak misafiri: muzakere eden ciftin menzilindeki ucuncu ajanlar da
  /// bilgi alir.
  void applyEavesdropping(int agent_a, int agent_b);

  /// P2'ye gore koordinasyonun KIMLER arasinda yapilacagini belirleyip yurutur.
  /// (P6 "ne zaman", P2 "kimler arasinda" sorusunu cevaplar.)
  void runCoordination(const std::vector<std::pair<int, int>> & pairs);

  /// P2a - tam merkezi: merkez tum ajanlarin durumunu gorur, comm-range
  /// gerektirmeden kuresel dengeleme yapar.
  void runCentralCoordination();

  /// P2b - hiyerarsik hibrit: menzilde birbirine bagli ajanlar bir kume olusturur,
  /// kumeye gecici bir lider secilir ve karar lider araciligiyla verilir.
  void runHierarchicalCoordination(const std::vector<std::pair<int, int>> & pairs);

  /// Iki ajan arasinda tek bir dengeleme adimi (devralma + takas degerlendirmesi).
  /// P2'nin uc secenegi de sonunda bunu cagirir; degisen yalnizca hangi ciftlerin
  /// secildigi ve bunun icin ne kadar iletisim gerektigidir.
  bool rebalancePair(int agent_a, int agent_b);

  /// Canli ajanlar arasindan kalan alani en fazla ve en az olan cifti bulur.
  std::pair<int, int> mostImbalancedPair(const std::vector<int> & agent_ids) const;

  /// P6a icin: bir sonraki yoklama zamani [s].
  double next_poll_time_{0.0};

  swarm_bt_core::ExperimentConfig config_;
  swarm_bt_core::SwarmState state_;
  swarm_bt_core::EncounterDetector detector_;
  swarm_bt_core::AreaSwapNegotiator negotiator_;
  swarm_bt_core::FailureInjector failure_injector_;
  std::unique_ptr<KinematicSim> sim_;
  EpisodeMetrics metrics_;
};

}  // namespace swarm_bt_sim

#endif  // SWARM_BT_SIM__EPISODE_RUNNER_HPP_
