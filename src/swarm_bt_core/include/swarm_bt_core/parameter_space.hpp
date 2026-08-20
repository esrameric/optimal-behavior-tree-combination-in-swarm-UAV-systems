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

/// Plan Bolum 9 - istege bagli genisletme: ucuncu (ve dorduncu) olcek degeri.
///
/// "Eger N=3 -> N=5 arasinda ilginc bir trend gorursen, bunun devam edip
/// etmedigini gormek icin ucuncu bir N degeri eklemeyi dusunebilirsin."
/// Trend gorüldu (atama kararliligi 16/16 kombinasyonda duşuyor), bu yuzden
/// genisletilmis olcek kumesi tanimli.
inline const std::vector<int> & extendedScaleValues()
{
  static const std::vector<int> kValues = {3, 5, 7, 10};
  return kValues;
}

/// Plan Bolum 9 - istege bagli genisletme: ORANTILI ALAN kontrol deneyi.
///
/// "Mission alanini N ile orantili buyuterek (drone basina duşen alan sabit
/// kalacak sekilde) saf olceklenebilirligi (karsilasma sikligi etkisi olmadan)
/// ayrica test edebilirsin -- bu, confound'u ortadan kaldiran temiz bir
/// kontrol grubu olur."
///
/// Alanin kenar uzunlugu sqrt(N / referans_N) ile carpilir; alan N ile
/// dogru orantili buyur, drone basina duşen alan SABIT kalir.
///
/// Onemli: bu islem alani \c sim.area_side uzerinden degistirir, gizli bir
/// N bagimliligi eklemez. ExperimentConfig::missionArea() N'den bagimsiz
/// kalmaya devam eder (Bolum 1 invaryanti).
ExperimentConfig withProportionalArea(
  const ExperimentConfig & base, int n_agents, int reference_agents);

/// Bir kombinasyonun olcekten arindirilmis kimligi ("..._N3" eki olmadan).
/// Iki olcegi eslestirmek icin kullanilir.
std::string combinationId(const ExperimentConfig & config);

/// Plan Bolum 4 - OFAT taramasinin baslangic noktasi olan BASELINE kombinasyon.
///
///   P2c  tam dagitik        - merkezi darbogaz yok, olcek sorusunun dogal zemini
///   P3c  CBBA               - dagitik mimariyle tutarli tek atama algoritmasi
///   P4b  ozdes dagitik BT   - her ajan ayni agaci calistirir
///   P5abc dogrudan + stigmerji + intent  - tam iletisim; eksiltmeler OFAT'ta olculur
///   P6c  saf olay-tetiklemeli - yalnizca comm-range girisinde karar
///
/// r_comm ve esik_degeri Bolum 1 ve 2.2'de kalibre edilmis degerlerdir.
ExperimentConfig baselineConfig();

/// OFAT taramasinda degistirilecek tek bir parametre ekseni.
struct ParameterAxis
{
  /// Eksenin adi ("P2", "P3", "P5", "P6").
  std::string name;
  /// Bu eksende denenecek secenekler (harf gosterimi).
  std::vector<std::string> options;
};

/// Plan Bolum 4 - OFAT eksenleri.
///
/// P4 bu listede YOKTUR: agacin yapisini degistirdigi icin ayri BT XML
/// dosyalariyla ifade edilir ve OFAT'ta ayri ele alinir (btArchitectureAxis).
const std::vector<ParameterAxis> & ofatAxes();

/// P4 ekseni; yapisal oldugu icin ayri tutulur.
const ParameterAxis & btArchitectureAxis();

/// Baseline'dan turetilen OFAT varyantlari: her eksende, baseline'dakinden
/// FARKLI her secenek icin bir kombinasyon. Baseline'in kendisi listenin
/// basinda yer alir (karsilastirma referansi).
///
/// Bu liste olcekle cogaltilmamistir; withScaleVariants() ile cogaltilir.
std::vector<ExperimentConfig> ofatVariants(const ExperimentConfig & baseline);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__PARAMETER_SPACE_HPP_
