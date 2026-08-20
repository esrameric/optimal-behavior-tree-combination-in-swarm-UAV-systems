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
/// HISTEREZIS: giris esigi r_comm, cikis esigi r_comm * (1 + hysteresis_ratio).
/// Histerezissiz bir esik, mesafe egrisi esige TEGET gectiginde saniyeler
/// icinde onlarca sahte giris uretir: bicerdover deseninde iki drone bitisik
/// sutunlarda paralel supururken aralarindaki mesafe tam olarak hucre boyutunun
/// kati kadardir ve hiz sapmasi yuzunden esigin iki yaninda salinir. Olculen
/// ornek: r_comm=20 m'de tek bir cift 130 kez "karsilasti" (her 2 saniyede bir,
/// hep d=20.00'da). Histerezis bunu tek bir karsilasmaya indirir ve fiziksel
/// olarak da dogrudur -- gercek bir telsiz baglantisi da, iki drone arasindaki
/// muzakere de her iki saniyede bir yeniden kurulmaz.
///
/// Ayni sinif hem Faz 1 (hafif simulator) hem Faz 2 (Gazebo) icin kullanilir;
/// degisen tek sey SwarmState icindeki pozisyonlarin kaynagidir.
class EncounterDetector
{
public:
  /// \param r_comm giris esigi [m]
  /// \param hysteresis_ratio cikis esigi carpani; 0 verilirse histerezis kapali
  ///   (chattering'e acik, yalnizca karsilastirma amacli kullanilmali).
  explicit EncounterDetector(double r_comm, double hysteresis_ratio = 0.1);

  double commRange() const {return r_comm_;}
  double hysteresisRatio() const {return hysteresis_ratio_;}
  /// Ciftin "menzilden cikti" sayilmasi icin asmasi gereken mesafe.
  double exitRange() const {return r_comm_ * (1.0 + hysteresis_ratio_);}

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
  double hysteresis_ratio_;
  std::set<std::pair<int, int>> in_range_;
  int total_encounters_{0};
};

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__ENCOUNTER_DETECTOR_HPP_
