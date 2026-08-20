#ifndef SWARM_BT_CORE__ASSIGNMENT_LOG_HPP_
#define SWARM_BT_CORE__ASSIGNMENT_LOG_HPP_

#include <cstdint>
#include <vector>

namespace swarm_bt_core
{

/// Bir atama degisikliginin sebebi (plan Bolum 6).
enum class AssignmentReason : std::uint8_t
{
  kAreaSwap = 0,        ///< alan takasi (Bolum 2.2)
  kOrphanTakeover = 1,  ///< arizali drone'dan devralma (Bolum 2.3)
  kJointScan = 2,       ///< ortak tarama (Bolum 2.2)
  kInitial = 3,         ///< baslangic atamasi (Bolum 2.1 adim 2)
  kFailure = 4          ///< ajanin kendisi arizalandi
};

/// Bir drone'un atanmis alaninin degistigi an.
///
/// Plan Bolum 6: "Atama kararliligi ve churn orani icin otomatik loglama ekle
/// (her swap olayinda rosbag2'ye event yaz)". Bu yapi o olayin kaydidir;
/// Faz 2'de dogrudan swarm_bt_msgs/AssignmentChange mesajina cevrilir.
struct AssignmentChangeEvent
{
  double time{0.0};
  int agent_id{-1};
  AssignmentReason reason{AssignmentReason::kInitial};
  /// Muzakere ortagi; yoksa -1.
  int peer_id{-1};
  /// Degisiklikten etkilenen hucre sayisi.
  int cells_changed{0};
  /// Degisiklik SONRASI bolge buyuklugu ve kalan hucre sayisi.
  int region_cells{0};
  int remaining_cells{0};
  /// Ajanin koşu boyunca kacinci atama degisikligi (1'den baslar).
  int change_index{0};
};

/// Atama degisikligi olaylarinin koşu boyunca biriktigi kayit.
///
/// Metrikler (atama kararliligi, churn orani) bu kayittan turetilebilir;
/// boylece "olculen sey" ile "kaydedilen sey" ayni kaynaktan gelir ve
/// birbirinden sapamaz.
class AssignmentLog
{
public:
  void record(const AssignmentChangeEvent & event) {events_.push_back(event);}

  const std::vector<AssignmentChangeEvent> & events() const {return events_;}
  std::size_t size() const {return events_.size();}
  bool empty() const {return events_.empty();}

  /// Kaydedilmis ama henuz yayinlanmamis olaylari dondurur ve isaretler.
  /// Faz 2'de ROS2 dugumu her tick bunu cagirip topic'e basar.
  std::vector<AssignmentChangeEvent> drain();

  /// Verilen sebebe sahip olay sayisi.
  int countByReason(AssignmentReason reason) const;

  /// Ajan basina ortalama atama degisikligi (Bolum 6: atama kararliligi).
  double changesPerAgent(int agent_count) const;

  void clear();

private:
  std::vector<AssignmentChangeEvent> events_;
  std::size_t published_{0};
};

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__ASSIGNMENT_LOG_HPP_
