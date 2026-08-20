#ifndef SWARM_BT_CORE__AREA_SWAP_HPP_
#define SWARM_BT_CORE__AREA_SWAP_HPP_

#include <optional>
#include <vector>

#include "swarm_bt_core/swarm_state.hpp"

namespace swarm_bt_core
{

/// Bir alan takasi teklifi (plan Bolum 2.2: alan_takasi_teklif_et()).
struct SwapProposal
{
  /// Kalan alani FAZLA olan, devretmeyi teklif eden ajan.
  int proposer_id{-1};
  /// Kalan alani AZ olan, teklifi degerlendiren ajan.
  int receiver_id{-1};
  /// Devredilmesi teklif edilen hucreler.
  std::vector<int> offered_cells;

  /// Teklif edenin bu hucrelerden kurtularak kazandigi tur mesafesi [m].
  double proposer_gain{0.0};
  /// Alicinin bu hucreleri ustlenmesinin kendi turuna ekledigi mesafe [m].
  double receiver_cost{0.0};

  /// Takasin ikilinin TOPLAM kat edecegi mesafeyi azaltip azaltmadigi.
  bool reducesTotalDistance() const {return receiver_cost < proposer_gain;}
};

/// Karsilasma aninda alan takasi muzakeresi (plan Bolum 2.2).
///
/// Mekanizma bilincli olarak BASIT tutulmustur (planin talebi):
///
///  1. Tetikleme  : |kalan_alan_orani_A - kalan_alan_orani_B| > esik_degeri
///  2. Teklif     : kalan alani fazla olan ajan, aliciya EN YAKIN hucrelerinden
///                  ikiliyi dengeleyecek kadarini devretmeyi onerir.
///  3. Kabul/red  : alici, hucreleri ustlenmenin kendi turuna ekleyecegi
///                  mesafeyi, teklif edenin bunlardan kurtularak kazanacagi
///                  mesafeyle karsilastirir ve takas ikilinin TOPLAM kat
///                  edecegi mesafeyi azaltiyorsa kabul eder.
///
/// (3) plandaki "teklif alan taraf, takas sonrasi toplam kat edecegi mesafenin
/// azalip azalmadigina bakar" olcutunun iyi tanimli halidir: hucreleri devralan
/// taraf her zaman is ustlenir, dolayisiyla kendi mesafesi tek basina asla
/// azalmaz; anlamli olan, hucrelerin onlari daha ucuza kapatabilecek ajana
/// gecip gecmedigidir. Bkz. README Varsayimlar V11.
class AreaSwapNegotiator
{
public:
  /// \param swap_threshold esik_degeri: kalan alan orani farki esigi [0,1].
  explicit AreaSwapNegotiator(double swap_threshold);

  double swapThreshold() const {return swap_threshold_;}

  /// Iki ajanin kalan alan oranlari arasindaki mutlak fark.
  static double imbalance(const SwarmState & state, int agent_a, int agent_b);

  /// plan Bolum 2.2: kalan_alan_farki > esik_degeri
  bool imbalanceAboveThreshold(const SwarmState & state, int agent_a, int agent_b) const;

  /// Ajanin mevcut konumundan baslayarak kalan hucreleri sirayla gezmesinin
  /// tahmini toplam mesafesi [m]. Takas faydasinin olcusu budur.
  static double tourLength(const SwarmState & state, int agent_id);

  /// tourLength, ancak verilen hucreler bolgeye eklenmis gibi hesaplanir.
  static double tourLengthWith(
    const SwarmState & state, int agent_id, const std::vector<int> & extra_cells);

  /// tourLength, ancak verilen hucreler bolgeden cikarilmis gibi hesaplanir.
  static double tourLengthWithout(
    const SwarmState & state, int agent_id, const std::vector<int> & removed_cells);

  /// Dengesizlik esigi asilmissa bir teklif kurar; asilmamissa bos doner.
  std::optional<SwapProposal> buildProposal(
    const SwarmState & state, int agent_a, int agent_b) const;

  /// Teklifi uygular: hucreler devredilir, iki ajanin da rotasi yeniden
  /// planlanir ve atama degisiklik sayaclari artirilir (Bolum 6 metrigi).
  void apply(SwarmState * state, const SwapProposal & proposal) const;

private:
  double swap_threshold_;
};

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__AREA_SWAP_HPP_
