#ifndef SWARM_BT_CORE__MISSION_AREA_HPP_
#define SWARM_BT_CORE__MISSION_AREA_HPP_

#include <vector>

#include "swarm_bt_core/geometry.hpp"

namespace swarm_bt_core
{

/// Gorev alani ve uzerindeki tarama gridi.
///
/// Alan, kenar uzunlugu \c cell_size olan kare hucrelere bolunur. Hucreler
/// dogrusal kimlikle (id = row * cols + col) adreslenir; alt-bolge atamasi,
/// stigmerji haritasi ve feromon haritasi ayni id uzayini paylasir.
///
/// Alan N'den bagimsiz SABIT tutulur (plan Bolum 1): N=5'te dogal olarak daha
/// sik karsilasma olusmasi, N=3 ile karsilastirmanin temel dayanagidir.
class MissionArea
{
public:
  MissionArea() = default;
  MissionArea(double width, double height, double cell_size);

  double width() const {return width_;}
  double height() const {return height_;}
  double cellSize() const {return cell_size_;}

  int cols() const {return cols_;}
  int rows() const {return rows_;}
  int cellCount() const {return cols_ * rows_;}

  /// Alanin kenar uzunlugu (kare olmayan alanlarda kisa kenar);
  /// r_comm kalibrasyonu buna gore yapilir (plan Bolum 1).
  double referenceSide() const;

  int cellId(int col, int row) const {return row * cols_ + col;}
  int colOf(int cell_id) const {return cell_id % cols_;}
  int rowOf(int cell_id) const {return cell_id / cols_;}

  bool validCell(int cell_id) const {return cell_id >= 0 && cell_id < cellCount();}

  /// Hucre merkezinin dunya koordinati (waypoint hedefi olarak kullanilir).
  Vec2 cellCenter(int cell_id) const;

  /// Konumu iceren hucre; alan disindaysa -1.
  int cellAt(const Vec2 & p) const;

  bool contains(const Vec2 & p) const;

  /// Hucre kimliklerini boustrophedon (lawn-mower) tarama sirasina dizer:
  /// sutun sutun ilerlenir, her sutunda satir yonu ters cevrilir (bicerdover).
  /// Satir yonu sutunun MUTLAK indeksine degil, sureste kacinci sirada
  /// tarandigina bakar; boylede ters yonlu taramalarda da ardisik waypoint'ler
  /// komsu kalir.
  ///
  /// \param reverse_columns true ise sutunlar sagdan sola taranir. Bitisik
  ///   ajanlara zit yon verildiginde suru duzeyinde serpantin desen olusur ve
  ///   komsular ortak sinirlarinda bulusur (bkz. assignEqualStrips).
  std::vector<int> boustrophedonOrder(
    std::vector<int> cell_ids, bool reverse_columns = false) const;

private:
  double width_{0.0};
  double height_{0.0};
  double cell_size_{1.0};
  int cols_{0};
  int rows_{0};
};

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__MISSION_AREA_HPP_
