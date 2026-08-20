#include "swarm_bt_core/mission_area.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <vector>

namespace swarm_bt_core
{

MissionArea::MissionArea(double width, double height, double cell_size)
: width_(width), height_(height), cell_size_(cell_size)
{
  if (width <= 0.0 || height <= 0.0 || cell_size <= 0.0) {
    throw std::invalid_argument("MissionArea: genislik/yukseklik/hucre boyutu pozitif olmali");
  }
  cols_ = static_cast<int>(std::floor(width / cell_size));
  rows_ = static_cast<int>(std::floor(height / cell_size));
  if (cols_ <= 0 || rows_ <= 0) {
    throw std::invalid_argument("MissionArea: hucre boyutu alandan buyuk");
  }
}

double MissionArea::referenceSide() const
{
  return std::min(width_, height_);
}

Vec2 MissionArea::cellCenter(int cell_id) const
{
  if (!validCell(cell_id)) {
    throw std::out_of_range("MissionArea::cellCenter: gecersiz hucre kimligi");
  }
  return Vec2{
    (colOf(cell_id) + 0.5) * cell_size_,
    (rowOf(cell_id) + 0.5) * cell_size_};
}

int MissionArea::cellAt(const Vec2 & p) const
{
  if (!contains(p)) {
    return -1;
  }
  const int col = std::min(cols_ - 1, static_cast<int>(p.x / cell_size_));
  const int row = std::min(rows_ - 1, static_cast<int>(p.y / cell_size_));
  return cellId(col, row);
}

bool MissionArea::contains(const Vec2 & p) const
{
  return p.x >= 0.0 && p.y >= 0.0 &&
         p.x <= cols_ * cell_size_ && p.y <= rows_ * cell_size_;
}

std::vector<int> MissionArea::boustrophedonOrder(
  std::vector<int> cell_ids, bool reverse_columns) const
{
  std::map<int, std::vector<int>> by_column;
  for (const int cell_id : cell_ids) {
    by_column[colOf(cell_id)].push_back(cell_id);
  }

  std::vector<int> columns;
  columns.reserve(by_column.size());
  for (const auto & entry : by_column) {
    columns.push_back(entry.first);
  }
  if (reverse_columns) {
    std::reverse(columns.begin(), columns.end());
  }

  std::vector<int> ordered;
  ordered.reserve(cell_ids.size());
  for (std::size_t rank = 0; rank < columns.size(); ++rank) {
    auto & cells = by_column[columns[rank]];
    std::sort(
      cells.begin(), cells.end(),
      [this](int lhs, int rhs) {return rowOf(lhs) < rowOf(rhs);});
    // Tarama sirasindaki tek numarali sutunlarda yon ters: bicerdover deseni.
    if (rank % 2 == 1) {
      std::reverse(cells.begin(), cells.end());
    }
    ordered.insert(ordered.end(), cells.begin(), cells.end());
  }
  return ordered;
}

}  // namespace swarm_bt_core
