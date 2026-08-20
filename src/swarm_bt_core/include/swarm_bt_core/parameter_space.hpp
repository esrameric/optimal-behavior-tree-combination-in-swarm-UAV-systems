#ifndef SWARM_BT_CORE__PARAMETER_SPACE_HPP_
#define SWARM_BT_CORE__PARAMETER_SPACE_HPP_

#include <string>
#include <vector>

#include "swarm_bt_core/experiment_config.hpp"

namespace swarm_bt_core
{

/// Plan Bolum 3 - test edilecek olcek degerleri.
///
/// Bu calismanin ikinci arastirma sorusu tam olarak bu iki deger arasindaki
/// farki soruyor: ayni kombinasyon N=3'ten N=5'e gecince nasil davraniyor?
/// Gorev alani sabit tutuldugu icin (Bolum 1) N=5 ayni alanda daha yogun ucus
/// ve daha sik karsilasma demektir.
inline const std::vector<int> & scaleValues()
{
  static const std::vector<int> kValues = {3, 5};
  return kValues;
}

/// Bir kombinasyonu her olcek degeri icin cogaltir.
///
/// Plan Bolum 3: "her finalist kombinasyon iki N degerinde de koşulacak".
/// Deney kimlikleri yalnizca _N eki ile ayrisir; boylece ayni kombinasyonun
/// iki olcegi metrik tablosunda eslesebilir (Bolum 6 N-duyarlilik skoru).
std::vector<ExperimentConfig> withScaleVariants(const ExperimentConfig & base);

/// Verilen kombinasyon listesini olcek degerleriyle cogaltir.
std::vector<ExperimentConfig> withScaleVariants(const std::vector<ExperimentConfig> & bases);

/// Bir kombinasyonun olcekten arindirilmis kimligi ("..._N3" eki olmadan).
/// Iki olcegi eslestirmek icin kullanilir.
std::string combinationId(const ExperimentConfig & config);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__PARAMETER_SPACE_HPP_
