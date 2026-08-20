#ifndef SWARM_BT_SIM__KINEMATIC_SIM_HPP_
#define SWARM_BT_SIM__KINEMATIC_SIM_HPP_

#include <vector>

#include <swarm_bt_core/swarm_state.hpp>

#include "swarm_bt_sim/position_source.hpp"

namespace swarm_bt_sim
{

/// Faz 1 hafif simulatorun sayisal parametreleri.
struct KinematicSimConfig
{
  /// Nominal ilerleme hizi [m/s]. Ivme/rüzgar modellenmez (plan: kapsam disi).
  double speed{10.0};
  /// Ajan basina hiz sapmasi orani; her drone U(1-j, 1+j) carpani alir.
  /// Sifir olursa ajanlar rijit formasyonda kilitlenir ve hic karsilasma dogmaz.
  double speed_jitter{0.05};
  /// Hiz sapmasinin tohumu; ayni tohum ayni koşuyu uretir (tekrarlanabilirlik).
  int seed{0};
  /// Tick suresi [s].
  double dt{0.1};
  /// Waypoint'e varmis sayilma yaricapi [m].
  double waypoint_tolerance{0.5};
  /// Taranan hucreye birakilan ilgi feromonu miktari.
  double interest_deposit{1.0};
  /// Gorev zaman siniri [s]; asilirsa koşu biter (plan Bolum 2.1 adim 6).
  double time_limit{3000.0};
};

/// Gazebo'nun fizik motorunu kullanmayan 2D kinematik surus simulatoru.
///
/// Kapsam (plan Bolum 0 "Faz 1 Hafif Simulator"):
///   - her drone icin 2D konum + sabit hiz ile waypoint takibi
///   - taranan hucrelerin stigmerji haritasina islenmesi + feromon birakma
///   - feromon haritasinin tick basina sonumlenmesi
/// Kapsam disi: gercek fizik (ivme, ruzgar, carpisma dinamigi), 3D yukseklik.
///
/// Comm-range giris tespiti bilincli olarak burada degil, swarm_bt_core
/// icindeki EncounterDetector'dadir: ayni kod Faz 2'de (Gazebo) de kullanilir.
class KinematicSim : public IPositionSource
{
public:
  KinematicSim(swarm_bt_core::SwarmState & state, const KinematicSimConfig & config);

  /// Bir tick ilerletir: ajanlari HEDEFLERINE dogru hareket ettirir, varilan
  /// hucreleri isaretler, feromonu sonumler ve simulasyon saatini ilerletir.
  ///
  /// Hedefi kim secer: BT (ScanNextCell dugumu). Bu sinif yalnizca ucus
  /// katmanidir; hedefi olmayan ajan yerinde bekler.
  void step() override;

  /// Kapsama tamamlandi veya zaman siniri asildi.
  bool finished() const override;

  int tickCount() const override {return tick_count_;}
  const KinematicSimConfig & config() const {return config_;}

private:
  void moveAgent(swarm_bt_core::AgentState & agent);

  /// Ajanin efektif hizi [m/s] (nominal hiz x tohumlanmis sapma carpani).
  double speedOf(int agent_id) const;

  swarm_bt_core::SwarmState * state_;
  KinematicSimConfig config_;
  /// Ajan basina hiz carpani; kurucu tarafindan bir kez cekilir.
  std::vector<double> speed_factors_;
  int tick_count_{0};
};

}  // namespace swarm_bt_sim

#endif  // SWARM_BT_SIM__KINEMATIC_SIM_HPP_
