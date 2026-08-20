#ifndef SWARM_BT_CORE__PHEROMONE_GRID_HPP_
#define SWARM_BT_CORE__PHEROMONE_GRID_HPP_

#include <vector>

namespace swarm_bt_core
{

/// Grid tabanli feromon haritasi (stigmerji altligi).
///
/// Plan Bolum 2.1'de taranan hucreler stigmerji haritasina islenir; Bolum 2.2'de
/// ise "sinir bolgesinde feromon yuksek mi" kosulu sorulur. Bu sinif her iki
/// kullanim icin de ortak altligi saglar: hucre basina skaler yogunluk,
/// birakma (deposit) ve tick basina ussel sonumleme (decay).
///
/// decay_rate = 0 -> sonumleme yok (kalici iz, "ziyaret edildi" katmani icin)
/// decay_rate > 0 -> her tick yogunluk (1 - decay_rate) ile carpilir
class PheromoneGrid
{
public:
  PheromoneGrid() = default;
  PheromoneGrid(int cell_count, double decay_rate);

  int cellCount() const {return static_cast<int>(values_.size());}
  double decayRate() const {return decay_rate_;}

  double at(int cell_id) const;
  void deposit(int cell_id, double amount);
  void set(int cell_id, double value);

  /// Tum hucreleri bir tick sonumler. Yogunluk kEpsilon altina duserse sifirlanir
  /// (kayan nokta artiklarinin "yuksek feromon" kosulunu kirletmesini onler).
  void decay();

  double total() const;
  double maxValue() const;

  /// Verilen hucre kumesindeki ortalama yogunluk; kume bossa 0.
  double meanOver(const std::vector<int> & cell_ids) const;

  void reset();

  static constexpr double kEpsilon = 1e-9;

private:
  std::vector<double> values_;
  double decay_rate_{0.0};
};

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__PHEROMONE_GRID_HPP_
