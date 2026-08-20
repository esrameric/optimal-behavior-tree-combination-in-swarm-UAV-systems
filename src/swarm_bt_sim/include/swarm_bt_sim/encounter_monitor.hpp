#ifndef SWARM_BT_SIM__ENCOUNTER_MONITOR_HPP_
#define SWARM_BT_SIM__ENCOUNTER_MONITOR_HPP_

#include <vector>

#include <swarm_bt_core/encounter_detector.hpp>
#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_core/swarm_state.hpp>

namespace swarm_bt_sim
{

/// Komsu pozisyonlarini kontrol edip comm-range giris olayi firlatan izleyici.
///
/// Plan Bolum 7: "Karsilasma olayi tespiti icin basit bir mesafe-kontrolu
/// node'u yaz (her tick'te ya da periyodik olarak komsu pozisyonlarini kontrol
/// eder, r_comm esigini gecenler icin event firlatir) -- hem Faz 1 hem Faz 2'de
/// AYNI node kodu kullanilmali, sadece pozisyon verisinin kaynagi degisir."
///
/// Bu sinif o node'un CEKIRDEGIDIR ve ROS'a bagimli degildir: pozisyonlar
/// disaridan setPosition() ile verilir. encounter_detector_node yalnizca ince
/// bir ROS sarmalayicidir (abonelikten setPosition, update ciktisindan topic).
/// Tespit mantigi swarm_bt_core::EncounterDetector'dir -- Faz 1'in koşu
/// dongusunun kullandigi sinifin ta kendisi.
class EncounterMonitor
{
public:
  /// \param agent_count izlenecek drone sayisi
  /// \param config r_comm, histerezis ve P6 tetikleme modelini tasir
  EncounterMonitor(int agent_count, const swarm_bt_core::ExperimentConfig & config);

  /// Bir drone'un guncel konumunu bildirir (pozisyon kaynagindan cagrilir).
  void setPosition(int agent_id, double x, double y);

  /// Bir drone'un arizali oldugunu bildirir; arizali droneler karsilasma
  /// uretmez (Bolum 2.3).
  void setAlive(int agent_id, bool alive);

  /// Kontrolu calistirir ve YENI karsilasmalari dondurur.
  ///
  /// P6a (periyodik yoklama) secilmisse kontrol yalnizca poll_period'da bir
  /// yapilir; aradaki cagrilar bos doner.
  std::vector<swarm_bt_core::EncounterEvent> update(double time);

  /// Bolum 6 metrigi: koşu basina toplam comm-range giris sayisi.
  int totalEncounters() const {return detector_.totalEncounters();}

  /// Kontrolun kac kez calistigi (P6'nin yoklama maliyeti).
  int checkCount() const {return check_count_;}

  int agentCount() const {return state_.agentCount();}

private:
  swarm_bt_core::ExperimentConfig config_;
  swarm_bt_core::SwarmState state_;
  swarm_bt_core::EncounterDetector detector_;
  double next_poll_time_{0.0};
  int check_count_{0};
};

}  // namespace swarm_bt_sim

#endif  // SWARM_BT_SIM__ENCOUNTER_MONITOR_HPP_
