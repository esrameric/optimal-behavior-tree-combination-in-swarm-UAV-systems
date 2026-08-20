#ifndef SWARM_BT_SIM__POSITION_SOURCE_HPP_
#define SWARM_BT_SIM__POSITION_SOURCE_HPP_

#include <swarm_bt_core/swarm_state.hpp>

namespace swarm_bt_sim
{

/// Ajan konumlarinin nereden geldigini soyutlar.
///
/// Plan Bolum 7: karsilasma tespiti ve BT karar mantigi "hem Faz 1 hem Faz 2'de
/// AYNI node kodu" olmali, "sadece pozisyon verisinin kaynagi degisir". Bu
/// arayuz o cumlenin kodda karsiligidir:
///
///   Faz 1  KinematicSim         -> 2D kinematik entegrasyon (fizik yok)
///   Faz 2  GazeboPositionSource -> Gazebo'dan gelen gercek gövde konumlari
///
/// Iki durumda da BT hedefi AgentState::target_cell'e yazar; kaynak, ajani o
/// hedefe dogru hareket ettirir ve varista AgentState::at_target'i isaretler.
class IPositionSource
{
public:
  virtual ~IPositionSource() = default;

  /// Bir tick ilerletir: konumlari gunceller, varislari isaretler, simulasyon
  /// saatini ilerletir.
  virtual void step() = 0;

  /// Kapsama tamamlandi ya da zaman siniri asildi.
  virtual bool finished() const = 0;

  /// Islenen tick sayisi (tick maliyeti metrigi).
  virtual int tickCount() const = 0;
};

}  // namespace swarm_bt_sim

#endif  // SWARM_BT_SIM__POSITION_SOURCE_HPP_
