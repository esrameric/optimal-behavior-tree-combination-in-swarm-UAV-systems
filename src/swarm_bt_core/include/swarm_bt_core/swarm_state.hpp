#ifndef SWARM_BT_CORE__SWARM_STATE_HPP_
#define SWARM_BT_CORE__SWARM_STATE_HPP_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "swarm_bt_core/assignment_log.hpp"
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

  /// BT'nin komut verdigi hedef hucre; -1 ise hedef yok (ajan bekler).
  ///
  /// Gorev bolusumu gercek bir sistemdeki gibi: BT "su hucreye git" der,
  /// ucus katmani (KinematicSim) ucurur. Bu ayrim P4'un davranisa yansimasi
  /// icin sart -- olay-gudumlu agac (P4c) calisan bir hareketi KESEBILMELI.
  int target_cell{-1};
  /// Ucus katmani hedefe varildigini boyle bildirir.
  bool at_target{false};

  /// Bolgenin sutunlarinin hangi yonde tarandigi. Bitisik ajanlara zit yon
  /// verilir (suru duzeyinde serpantin); takas sonrasi bolge yeniden
  /// siralanirken ayni yon korunur.
  bool sweep_reversed{false};

  /// Bolum 6 metrigi (atama kararliligi): atanmis alanin kac kez degistigi.
  int assignment_changes{0};

  /// Toplam kat edilen mesafe; takas tekliflerinin fayda hesabinda (Bolum 2.2)
  /// ve iletisim/hareket maliyeti raporlamasinda kullanilir.
  double distance_travelled{0.0};

  /// Ajanin taranmis oldugunu BILDIGI hucreler (kendi yerel BT bellegi).
  ///
  /// Stigmerji (P5b) acikken ajan cevredeki izleri okuyabildigi icin bu bilgi
  /// kuresel haritayla ayni olur. Kapaliyken ajan yalnizca kendi taradigi
  /// hucreleri bilir; baskasinin bilgisini ancak dogrudan mesaj (P5a) ya da
  /// kulak misafiri (P5d) yoluyla ogrenir. Bilmedigi bir hucreyi tekrar tarar --
  /// stigmerjinin olculebilir faydasi tam olarak budur.
  std::vector<uint8_t> known_visited;
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

  /// Hucre gercekten tarandi mi (yer gercekligi; kapsama olcumu bunu kullanir).
  bool isVisited(int cell_id) const;
  void markVisited(int cell_id);

  /// Stigmerji (P5b) etkin mi. Etkinse ajanlarin bilgisi kuresel haritayla ayni.
  bool stigmergyEnabled() const {return stigmergy_;}
  void setStigmergyEnabled(bool enabled) {stigmergy_ = enabled;}

  /// Ajan bu hucrenin tarandigini BILIYOR mu (rota planlamasi bunu kullanir).
  bool knowsVisited(int agent_id, int cell_id) const;

  /// Hucreyi tarar: yer gercekligi ve tarayan ajanin bilgisi guncellenir.
  void markVisitedBy(int agent_id, int cell_id);

  /// Iki ajanin taranmis-hucre bilgisini birlestirir (dogrudan mesaj / kulak
  /// misafiri ile stigmerji senkronizasyonu, plan Bolum 2.2).
  /// \return karsi taraftan ogrenilen YENI hucre sayisi (iletisim yuku olcusu).
  int shareKnowledge(int agent_a, int agent_b);

  /// Ajanin bolgesinde, KENDI BILGISINE gore taranmamis kalan hucre sayisi.
  int remainingCellsKnown(int agent_id) const;

  /// Ajanin bolgesinde henuz taranmamis hucre sayisi.
  int remainingCells(int agent_id) const;

  /// Ajanin bolgesindeki taranmamis hucreler.
  std::vector<int> remainingCellIds(int agent_id) const;

  /// Ajanin bolgesini kendi supurme yonuyle yeniden siralar ve siradaki
  /// waypoint'i basa alir. Takas sonrasi rota yeniden planlamasi icin.
  void resequenceRegion(int agent_id);

  /// Atama degisikligi olay kaydi (plan Bolum 6).
  const AssignmentLog & assignmentLog() const {return assignment_log_;}
  AssignmentLog & assignmentLog() {return assignment_log_;}

  /// Bir atama degisikligini kaydeder VE ajanin sayacini artirir.
  ///
  /// Atama degisikliginin TEK gecis noktasi budur: sayac ile olay kaydi
  /// birbirinden sapamasin diye ikisi ayni cagride guncellenir. Bolum 6'daki
  /// atama kararliligi ve churn orani metrikleri bu kayittan turer.
  void recordAssignmentChange(
    int agent_id, AssignmentReason reason, int peer_id, int cells_changed);
  /// Ajanin bolgesinde taranmamis hucre orani [0,1]; bolge bossa 0.
  double remainingRatio(int agent_id) const;

  /// Arizalanan drone'dan devrolan, henuz kimseye atanmamis hucreler
  /// (plan Bolum 2.3: "kalan alaninin nasil devralindigini gozlemle").
  const std::vector<int> & orphanedCells() const {return orphaned_cells_;}

  /// Ajani arizalandirir: canli bayragi duşer, bolgesindeki TARANMAMIS hucreler
  /// sahipsiz havuza tasinir. Taranmis hucreler kimseye lazim degil, atilir.
  void failAgent(int agent_id);

  /// Canli bir ajanin sahipsiz hucrelerden bir kismini devralmasi.
  void claimOrphanedCells(int agent_id, const std::vector<int> & cells);

  /// Tum ajanlarin bolgeleri VE sahipsiz havuz tarandiginda true.
  bool coverageComplete() const;

  /// Bolum 6 metrigi (kapsama dengesizligi): ajanlar arasi "kalan alan"
  /// degerlerinin standart sapmasi.
  double coverageImbalance() const;

  /// Alani N dikey serite esit boler ve her ajana bir serit atar (P3a: statik).
  /// Ajanlar kendi seritlerinin ilk waypoint'ine yerlestirilir.
  void assignEqualStrips();

  /// Ajanlari alan icinde tohumlanmis rastgele konumlara yerlestirir.
  /// Bolge atamasina dokunmaz; yalnizca kalkis geometrisini belirler.
  void randomizeLaunchPositions(int seed);

  /// Gorev alanina tohumlanmis rastgele ilgi noktalari serpistirir.
  /// Feromon yalnizca bu hucrelerde birakilir (bkz. ExperimentConfig).
  void placeInterestPoints(int count, int seed);

  /// Ilgi noktalarini dogrudan verilen hucrelere yerlestirir (testler ve
  /// tekrarlanabilir senaryo kurulumu icin).
  void placeInterestPointsAt(std::vector<int> cell_ids);

  /// Hucrede ilgi noktasi var mi.
  bool hasInterestPoint(int cell_id) const;

  const std::vector<int> & interestPoints() const {return interest_points_;}

private:
  MissionArea area_;
  std::vector<AgentState> agents_;
  PheromoneGrid visited_;
  PheromoneGrid interest_;
  std::vector<int> orphaned_cells_;
  std::vector<int> interest_points_;
  AssignmentLog assignment_log_;
  bool stigmergy_{true};
  double time_{0.0};
};

/// Deney konfigurasyonundan baslangic suru durumu uretir.
///
/// Alan ExperimentConfig::missionArea() uzerinden gelir; bu sayede N ile alan
/// arasinda hicbir bagimlilik olusamaz (plan Bolum 1). Kalkis konumlari
/// config.sim.random_launch acikken \p seed'den turetilir; bolge atamasi
/// config.p3'e gore yapilir.
SwarmState makeSwarmState(const ExperimentConfig & config, int seed);
SwarmState makeSwarmState(const ExperimentConfig & config);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__SWARM_STATE_HPP_
