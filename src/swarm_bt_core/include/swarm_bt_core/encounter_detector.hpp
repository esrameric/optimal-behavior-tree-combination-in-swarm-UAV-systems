#ifndef SWARM_BT_CORE__ENCOUNTER_DETECTOR_HPP_
#define SWARM_BT_CORE__ENCOUNTER_DETECTOR_HPP_

#include <set>
#include <utility>
#include <vector>

#include "swarm_bt_core/swarm_state.hpp"

namespace swarm_bt_core
{

/// Iki drone'un iletisim menziline girmesiyle tetiklenen karsilasma olayi.
struct EncounterEvent
{
  int agent_a{-1};
  int agent_b{-1};
  double time{0.0};
  double distance{0.0};
};

/// Comm-range giris olaylarini tespit eder (plan Bolum 2.1 adim 5, Bolum 7).
///
/// Olay YUKSELEN KENARDA uretilir: bir cift, onceki kontrolde menzil disindayken
/// bu kontrolde menzile girdiyse tek bir olay dogar. Menzil icinde kaldiklari
/// surece tekrar olay uretilmez; ciftin menzilden cikmasi durumu sifirlar.
/// Bu, Bolum 6'daki "karsilasma sikligi" metriginin (koşu basina comm-range
/// giris sayisi) dogru sayilmasi icin gereklidir.
///
/// Ayni sinif hem Faz 1 (hafif simulator) hem Faz 2 (Gazebo) icin kullanilir;
/// degisen tek sey SwarmState icindeki pozisyonlarin kaynagidir.
class EncounterDetector
{
public:
  explicit EncounterDetector(double r_comm);

  double commRange() const {return r_comm_;}

  /// Bir kontrol adimi calistirir ve YENI karsilasmalari dondurur.
  std::vector<EncounterEvent> update(const SwarmState & state);

  /// Bolum 6 metrigi: koşu basina toplam comm-range giris olayi sayisi.
  int totalEncounters() const {return total_encounters_;}

  /// Su an menzil icinde olan cift sayisi.
  std::size_t pairsInRange() const {return in_range_.size();}

  void reset();

private:
  static std::pair<int, int> orderedPair(int a, int b);

  double r_comm_;
  std::set<std::pair<int, int>> in_range_;
  int total_encounters_{0};
};

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__ENCOUNTER_DETECTOR_HPP_
