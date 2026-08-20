#ifndef SWARM_BT_CORE__SWARM_STATE_HPP_
#define SWARM_BT_CORE__SWARM_STATE_HPP_

#include <cstddef>
#include <vector>

#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/geometry.hpp"
#include "swarm_bt_core/mission_area.hpp"
#include "swarm_bt_core/pheromone_grid.hpp"

namespace swarm_bt_core
{

/// Tek bir keskif (scout) drone'unun durumu.
///
/// Suru homojen: rol ayrimi yok, tum ajanlar ayni tipte ve ayni gorevde.
struct AgentState
{
  int id{0};
  Vec2 position;
  double battery{1.0};
  bool alive{true};

  /// Atanmis hucre kimlikleri, tarama sirasinda (boustrophedon).
  std::vector<int> region;
  /// region icinde siradaki hedef hucrenin indeksi.
  std::size_t next_waypoint{0};

  /// Bolgenin sutunlarinin hangi yonde tarandigi. Bitisik ajanlara zit yon
  /// verilir (suru duzeyinde serpantin); takas sonrasi bolge yeniden
  /// siralanirken ayni yon korunur.
  bool sweep_reversed{false};

  /// Bolum 6 metrigi (atama kararliligi): atanmis alanin kac kez degistigi.
  int assignment_changes{0};

  /// Toplam kat edilen mesafe; takas tekliflerinin fayda hesabinda (Bolum 2.2)
  /// ve iletisim/hareket maliyeti raporlamasinda kullanilir.
  double distance_travelled{0.0};
};

/// Sürünün paylasilan dunya modeli.
///
/// Faz 1 (hafif kinematik simulator) ve Faz 2 (Gazebo+SITL) ayni yapiyi
/// kullanir; aralarindaki tek fark pozisyonlarin nasil guncellendigidir
/// (plan Bolum 7). Bu sayede BT node'lari ve karsilasma tespiti tek kod
/// tabanindan calisir.
class SwarmState
{
public:
  SwarmState() = default;
  SwarmState(const MissionArea & area, int agent_count, double pheromone_decay);

  const MissionArea & area() const {return area_;}

  std::vector<AgentState> & agents() {return agents_;}
  const std::vector<AgentState> & agents() const {return agents_;}
  AgentState & agent(int id);
  const AgentState & agent(int id) const;
  int agentCount() const {return static_cast<int>(agents_.size());}

  /// "Ziyaret edildi" stigmerji katmani (sonumlemesiz kalici iz).
  PheromoneGrid & visited() {return visited_;}
  const PheromoneGrid & visited() const {return visited_;}

  /// Ilgi feromonu (sonumlemeli) - sinir bolgesi ortak ilgi kosulunda kullanilir.
  PheromoneGrid & interest() {return interest_;}
  const PheromoneGrid & interest() const {return interest_;}

  double time() const {return time_;}
  void advanceTime(double dt) {time_ += dt;}
  /// Zamani dogrudan atar. Simulator, tick sayisindan turetilmis zamani
  /// (tick * dt) yazmak icin bunu kullanir: dt'nin tekrar tekrar toplanmasi
  /// kayan nokta kaymasi yaratir ve zaman siniri kontrolunu bir tick kaydirir.
  void setTime(double t) {time_ = t;}

  bool isVisited(int cell_id) const;
  void markVisited(int cell_id);

  /// Ajanin bolgesinde henuz taranmamis hucre sayisi.
  int remainingCells(int agent_id) const;

  /// Ajanin bolgesindeki taranmamis hucreler.
  std::vector<int> remainingCellIds(int agent_id) const;

  /// Ajanin bolgesini kendi supurme yonuyle yeniden siralar ve siradaki
  /// waypoint'i basa alir. Takas sonrasi rota yeniden planlamasi icin.
  void resequenceRegion(int agent_id);
  /// Ajanin bolgesinde taranmamis hucre orani [0,1]; bolge bossa 0.
  double remainingRatio(int agent_id) const;

  /// Tum ajanlarin bolgelerindeki taranmamis hucreler bittiginde true.
  bool coverageComplete() const;

  /// Bolum 6 metrigi (kapsama dengesizligi): ajanlar arasi "kalan alan"
  /// degerlerinin standart sapmasi.
  double coverageImbalance() const;

  /// Alani N dikey serite esit boler ve her ajana bir serit atar (P3a: statik).
  /// Ajanlar kendi seritlerinin ilk waypoint'ine yerlestirilir.
  void assignEqualStrips();

private:
  MissionArea area_;
  std::vector<AgentState> agents_;
  PheromoneGrid visited_;
  PheromoneGrid interest_;
  double time_{0.0};
};

/// Deney konfigurasyonundan baslangic suru durumu uretir.
///
/// Alan ExperimentConfig::missionArea() uzerinden gelir; bu sayede N ile alan
/// arasinda hicbir bagimlilik olusamaz (plan Bolum 1).
SwarmState makeSwarmState(const ExperimentConfig & config);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__SWARM_STATE_HPP_
