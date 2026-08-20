#ifndef SWARM_BT_SIM__EPISODE_RUNNER_HPP_
#define SWARM_BT_SIM__EPISODE_RUNNER_HPP_

#include <memory>

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
