#ifndef SWARM_BT_CORE__GEOMETRY_HPP_
#define SWARM_BT_CORE__GEOMETRY_HPP_

#include <cmath>

namespace swarm_bt_core
{

/// 2B konum/vektor.
///
/// Plan geregi Faz 1 kapsami 2D'dir: amac ucus dinamigini degil BT karar
/// mantigini test etmek oldugu icin yukseklik bilincli olarak kapsam disi.
struct Vec2
{
  double x{0.0};
  double y{0.0};
};

inline Vec2 operator+(const Vec2 & a, const Vec2 & b)
{
  return Vec2{a.x + b.x, a.y + b.y};
}

inline Vec2 operator-(const Vec2 & a, const Vec2 & b)
{
  return Vec2{a.x - b.x, a.y - b.y};
}

inline Vec2 operator*(const Vec2 & v, double s)
{
  return Vec2{v.x * s, v.y * s};
}

inline double norm(const Vec2 & v)
{
  return std::sqrt(v.x * v.x + v.y * v.y);
}

inline double distance(const Vec2 & a, const Vec2 & b)
{
  return norm(a - b);
}

/// Birim vektor; sifir vektorde {0,0} doner (sifira bolme korumasi).
inline Vec2 normalized(const Vec2 & v)
{
  const double n = norm(v);
  if (n <= 1e-12) {
    return Vec2{0.0, 0.0};
  }
  return Vec2{v.x / n, v.y / n};
}

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__GEOMETRY_HPP_
