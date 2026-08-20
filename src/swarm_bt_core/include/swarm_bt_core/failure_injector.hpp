#ifndef SWARM_BT_CORE__FAILURE_INJECTOR_HPP_
#define SWARM_BT_CORE__FAILURE_INJECTOR_HPP_

#include <optional>
#include <random>
#include <vector>

#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/swarm_state.hpp"

namespace swarm_bt_core
{

/// Plan Bolum 2.3 - opsiyonel surpriz olay.
///
/// Bir koşuda rastgele bir drone'u gorev ORTASINDA arizalandirir (BT'yi
/// FAILURE'a zorlar); amac kalan alaninin nasil devralindigini gozlemlemektir.
/// Bu, robustness testinin ayri bir boyutu oldugu kadar, calismanin asil
/// sorusu icin de kritik: arizadan dogan buyuk dengesizlik, yeniden-atama
/// mekanizmasini gercekten calistiran ana kaynaktir.
///
/// Tetikleme zamani:
///   config.time >= 0  -> tam o simulasyon zamaninda
///   config.time <  0  -> genel kapsama %50'ye ulastiginda ("gorev ortasi").
///     Gorev suresi onceden bilinmedigi icin zaman yerine ilerleme kullanilir;
///     bu, N ve hiz degistiginde de "orta"yi dogru yakalar.
///
/// Arizalanacak drone:
///   config.agent_id >= 0 -> o drone
///   config.agent_id <  0 -> canli droneler arasindan tohumlanmis rastgele secim
class FailureInjector
{
public:
  FailureInjector(const FailureInjection & config, int seed);

  /// Her tick cagrilir. Kosul saglandiysa bir drone'u arizalandirir ve kimligini
  /// dondurur; aksi halde bos doner. En fazla bir kez tetiklenir.
  std::optional<int> update(SwarmState * state);

  bool triggered() const {return triggered_;}
  /// Arizalandirilan drone'un kimligi; henuz tetiklenmediyse -1.
  int failedAgent() const {return failed_agent_;}
  /// Arizanin gerceklestigi simulasyon zamani; henuz tetiklenmediyse -1.
  double failureTime() const {return failure_time_;}

  /// Genel kapsama orani esigi (config.time < 0 oldugunda kullanilir).
  static constexpr double kMidMissionCoverage = 0.5;

private:
  bool shouldTrigger(const SwarmState & state) const;
  int selectAgent(const SwarmState & state);

  FailureInjection config_;
  std::mt19937 rng_;
  bool triggered_{false};
  int failed_agent_{-1};
  double failure_time_{-1.0};
};

/// Genel kapsama orani: taranmis hucre sayisi / toplam hucre sayisi.
double globalCoverageRatio(const SwarmState & state);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__FAILURE_INJECTOR_HPP_
