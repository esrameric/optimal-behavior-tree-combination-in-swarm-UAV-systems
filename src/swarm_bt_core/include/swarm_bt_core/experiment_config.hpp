#ifndef SWARM_BT_CORE__EXPERIMENT_CONFIG_HPP_
#define SWARM_BT_CORE__EXPERIMENT_CONFIG_HPP_

#include <string>

#include "swarm_bt_core/mission_area.hpp"

namespace swarm_bt_core
{

/// P2 - Koordinasyon mimarisi.
enum class CoordinationArchitecture
{
  kCentral,             ///< P2a: tam merkezi
  kHierarchicalHybrid,  ///< P2b: hiyerarsik hibrit (gecici lider secilir)
  kDistributed          ///< P2c: tam dagitik
};

/// P3 - Baslangic alan atama algoritmasi.
enum class AllocationAlgorithm
{
  kStaticEqual,   ///< P3a: statik esit bolme
  kContractNet,   ///< P3b: Contract Net
  kCbba           ///< P3c: CBBA
};

/// P4 - BT mimarisi. Agacin dugum YAPISINI degistirdigi icin config ile degil,
/// ayri BT XML dosyalariyla ifade edilir (bkz. swarm_bt_core/bt_xml/).
enum class BtArchitecture
{
  kCentral,      ///< P4a: tek merkezi BT
  kDistributed,  ///< P4b: ozdes dagitik BT
  kEventDriven   ///< P4c: olay-gudumlu (Micros.BT tarzi)
};

/// P6 - Tetikleme modeli.
enum class TriggerModel
{
  kPeriodicPolling,  ///< P6a: periyodik yoklama
  kEveryTick,        ///< P6b: her tick kontrol
  kEventDriven       ///< P6c: saf olay-tetiklemeli (sadece comm-range girisinde)
};

/// P5 - Iletisim mekanizmalari (birlikte secilebilir, ornegin P5abc).
///
/// Blackboard ayri bir mekanizma DEGILDIR: her ajanin yerel BT bellegidir.
/// Buradaki bayraklar o bellegin NASIL doldurulacagini belirler.
struct CommunicationMechanisms
{
  bool direct_message{true};  ///< P5a: dogrudan mesaj (negotiation handshake)
  bool stigmergy{true};       ///< P5b: stigmerji (ziyaret hucre paylasimi)
  bool intent_broadcast{true};  ///< P5c: intent yayini (takas/plan duyurusu)
  bool eavesdrop{false};      ///< P5d: kulak misafiri (opsiyonel)

  /// "abc" gibi harf dizisi; hicbiri secili degilse "none".
  std::string toLetters() const;
  static CommunicationMechanisms fromLetters(const std::string & letters);
};

/// Simulasyonun sayisal parametreleri (davranissal olmayan).
struct SimulationParameters
{
  double area_side{400.0};        ///< kare gorev alani kenar uzunlugu [m]
  double cell_size{20.0};         ///< tarama hucresi kenar uzunlugu [m]
  double speed{10.0};             ///< nominal ucus hizi [m/s]
  /// Ajan basina hiz sapmasi (orani). Her drone hizini U(1-j, 1+j) carpani ile
  /// alir. Sifir birakilirsa tum ajanlar tam olarak ayni hizda ucar ve seritler
  /// boyunca RIJIT FORMASYONDA kilitlenir; bu durumda r_comm'un karsilasma
  /// sikligina hicbir etkisi olmaz (bkz. README Varsayimlar V8).
  double speed_jitter{0.05};
  double dt{0.1};                 ///< tick suresi [s]
  double time_limit{3000.0};      ///< gorev zaman siniri [s]
  double waypoint_tolerance{0.5};  ///< waypoint'e varmis sayilma yaricapi [m]
  double pheromone_decay{0.01};   ///< ilgi feromonu tick basina sonumleme orani
  /// Bir ILGI NOKTASI bulundugunda o hucreye birakilan feromon miktari.
  double interest_deposit{1.0};
  /// Gorev alanina tohumlanmis olarak serpistirilen ilgi noktasi sayisi.
  ///
  /// Feromon her taranan hucreye degil, yalnizca ilgi noktasi bulunan hucreye
  /// birakilir; aksi halde "sinirda ortak ilgi yuksek" kosulu (plan Bolum 2.2)
  /// yalnizca "sinir yakin zamanda tarandi" anlamina gelirdi ve ortak tarama
  /// dalini anlamsiz kilardi.
  int interest_points{12};
  double poll_period{5.0};        ///< P6a periyodik yoklama araligi [s]

  /// Droneler gorev alani icinde rastgele (tohumlanmis) konumlardan kalkar.
  ///
  /// Kapatilirsa her drone dogrudan kendi seridinin ilk waypoint'ine konur.
  /// O durumda alan atama problemi ONCEDEN COZULMUS olur: ihaleli algoritmalar
  /// (P3b Contract Net, P3c CBBA) esit serit bolmesini birebir yeniden uretir
  /// ve P3 ekseni hicbir sey olcmez. Ayrica koşular arasi tek rastgelelik hiz
  /// sapmasi kalir, bu da >=10 tekrarin anlamini zayiflatir.
  bool random_launch{true};

  /// Guvenlik yaricapi [m]. Iki drone bunun altina inerse "carpisma" sayilir.
  ///
  /// Faz 1 simulatoru carpisma DINAMIGI modellemez (plan: kapsam disi); bu
  /// olcum bir yakinlik ihlali sayacidir ve Bolum 6'daki "carpisma sayisi"
  /// metriginin kod-seviyesi karsiligidir. Faz 2'de Gazebo gercek carpismayi
  /// uretecek; iki olcumun ayni yonde hareket edip etmedigi karsilastirilacak.
  double safety_radius{5.0};

  /// Ortak tarama esigi: iki ajanin sinir bolgesindeki ortalama feromon bu
  /// degeri asarsa "birlikte tara" dali tetiklenir (plan Bolum 2.2).
  double joint_scan_threshold{0.25};
};

/// Bolum 2.3 - opsiyonel surpriz olay: bir drone'u gorev ortasinda arizalandirma.
struct FailureInjection
{
  bool enabled{false};
  double time{-1.0};   ///< arizanin tetiklenecegi simulasyon zamani [s]; <0 ise koşu ortasi
  int agent_id{-1};    ///< arizalanacak drone; <0 ise rastgele secilir
};

/// Tek bir deney koşusunun tam tanimi.
///
/// Plan Bolum 0 "Konfigurasyon Yonetimi": davranissal OLMAYAN parametreler
/// (P2, P3, P5, P6, r_comm, esik_degeri, N) YAML'da tutulur; P4 ise agacin
/// yapisini degistirdigi icin ayri BT XML dosyalariyla ifade edilir. P4 burada
/// yalnizca hangi XML dosyasinin yuklenecegini secmek uzere bulunur.
struct ExperimentConfig
{
  int n_agents{3};              ///< N in {3, 5}
  /// Iletisim menzili [m]. Bolum 1'de kalibre edildi: 400 m'lik alanin %15'i.
  /// Gerekce ve tarama verisi: experiments/calibration_rcomm.md
  double r_comm{60.0};
  /// Karsilasma tespitinde cikis esigi carpani (bkz. EncounterDetector).
  /// Sifirlanirsa esige teget gecen mesafe egrisi sahte karsilasma yagmuru uretir.
  double encounter_hysteresis{0.1};
  /// esik_degeri: dengesizlik takas esigi [0,1].
  ///
  /// Bolum 5/Faz 0'da BT karar katmani ve sutun granulerlikli takas ile YENIDEN
  /// kalibre edildi: 0.20 -> 0.10. Planin onerdigi 0.30 ve ilk kalibrasyondaki
  /// 0.20, takas etkinligini duşuruyor.
  /// Gerekce ve tarama verisi: experiments/calibration_threshold_bt.md
  double swap_threshold{0.10};

  CoordinationArchitecture p2{CoordinationArchitecture::kDistributed};
  AllocationAlgorithm p3{AllocationAlgorithm::kCbba};
  BtArchitecture p4{BtArchitecture::kDistributed};
  CommunicationMechanisms p5{};
  TriggerModel p6{TriggerModel::kEventDriven};

  SimulationParameters sim{};
  FailureInjection failure{};

  int repetitions{10};  ///< plan Bolum 5/Faz 1: kombinasyon x N basina >= 10 tekrar
  int seed{0};

  /// Plan Bolum 3 semasi: "P2b_P3c_P4c_P5bc_P6c_N3".
  ///
  /// Deney kimligi, calismanin BIRINCIL ANAHTARIDIR: config dosyalarinin adi,
  /// metrik tablosundaki satir anahtari ve rosbag2 kayitlarinin etiketi hep
  /// budur. Bu yuzden kimlikten konfigurasyona geri donuş de tanimli olmali
  /// (fromExperimentId), aksi halde bir metrik satirindan deneyi yeniden
  /// koşmak icin elle esleme gerekirdi.
  std::string experimentId() const;

  /// Deney kimligini ayristirip konfigurasyona cevirir.
  ///
  /// Kimlikte yer ALMAYAN alanlar (r_comm, esik_degeri, simulasyon sayisallari)
  /// \p defaults degerlerinde birakilir; kimlik yalnizca parametre uzayini
  /// (P2-P6) ve olcegi (N) tasir.
  ///
  /// Bicim bozuksa std::invalid_argument atar.
  static ExperimentConfig fromExperimentId(const std::string & id);
  static ExperimentConfig fromExperimentId(
    const std::string & id, const ExperimentConfig & defaults);

  /// Gorev alanini uretir.
  ///
  /// ALANIN TEK KAYNAGI BURASIDIR ve bilincli olarak n_agents'i KULLANMAZ:
  /// plan Bolum 1 geregi mission alani N degisse de buyumez. N=5'te ayni alanda
  /// daha yogun ucus, dolayisiyla daha sik karsilasma olusmasi, N=3 ile
  /// karsilastirmanin temel dayanagidir. (Alani N ile orantili buyutup saf
  /// olceklenebilirligi olcmek, plan Bolum 9'daki AYRI bir kontrol deneyidir.)
  MissionArea missionArea() const;

  /// Drone basina duşen ortalama tarama hucresi sayisi. Alan sabit oldugu icin
  /// bu deger N ile ters orantili azalir; olcek karsilastirmasinin niceliksel
  /// ifadesi budur.
  double cellsPerAgent() const;

  /// P4 seciminin karsilik geldigi BT XML dosya adi.
  std::string btXmlFileName() const;

  /// Degerlerin tutarliligini dogrular; bozuksa std::invalid_argument atar.
  void validate() const;

  std::string toYaml() const;
  static ExperimentConfig fromYamlString(const std::string & yaml_text);
  static ExperimentConfig fromYamlFile(const std::string & path);
};

// --- Enum <-> harf donusumleri (YAML ve deney kimliginde "a"/"b"/"c" kullanilir) ---
std::string toLetter(CoordinationArchitecture value);
std::string toLetter(AllocationAlgorithm value);
std::string toLetter(BtArchitecture value);
std::string toLetter(TriggerModel value);

CoordinationArchitecture coordinationFromLetter(const std::string & letter);
AllocationAlgorithm allocationFromLetter(const std::string & letter);
BtArchitecture btArchitectureFromLetter(const std::string & letter);
TriggerModel triggerModelFromLetter(const std::string & letter);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__EXPERIMENT_CONFIG_HPP_
