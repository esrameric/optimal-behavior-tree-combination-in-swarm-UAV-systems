#ifndef SWARM_BT_CORE__EXPERIMENT_CONFIG_HPP_
#define SWARM_BT_CORE__EXPERIMENT_CONFIG_HPP_

#include <string>

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
  double speed{10.0};             ///< sabit ucus hizi [m/s]
  double dt{0.1};                 ///< tick suresi [s]
  double time_limit{3000.0};      ///< gorev zaman siniri [s]
  double waypoint_tolerance{0.5};  ///< waypoint'e varmis sayilma yaricapi [m]
  double pheromone_decay{0.01};   ///< ilgi feromonu tick basina sonumleme orani
  double interest_deposit{1.0};   ///< taranan hucreye birakilan feromon miktari
  double poll_period{5.0};        ///< P6a periyodik yoklama araligi [s]
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
  double r_comm{60.0};          ///< iletisim menzili [m]
  double swap_threshold{0.30};  ///< esik_degeri: dengesizlik takas esigi [0,1]

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
  std::string experimentId() const;

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
