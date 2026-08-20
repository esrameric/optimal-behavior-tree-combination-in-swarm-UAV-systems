# Homojen Keşif Sürüsü — Ölçek-Duyarlılık Araştırması

Tek-rollü (scout) homojen bir drone sürüsünde, davranışsal planlama için
Behavior Tree (BT) + koordinasyon kombinasyonlarının ölçek duyarlılığını
inceleyen araştırma reposu.

## Araştırma Soruları

1. Tek-rollü (keşif/scout) homojen bir sürüde davranışsal planlama için optimal
   BT + koordinasyon kombinasyonu nedir?
2. **Aynı kombinasyon**, sürü büyüklüğü değiştiğinde (N=3 → N=5) nasıl farklı
   davranıyor? BT'nin karar mekanizması (özellikle dinamik yeniden-atama) ajan
   sayısı arttıkça daha kararsız/oynak mı (manipülasyona açık), yoksa daha
   kararlı mı (dayanıklı) hale geliyor?

Kaynak plan: `yapilacaklar_homojen.md` (repo dışı, kullanıcıda).

## Teknoloji Yığını

| Katman | Seçim |
|---|---|
| İşletim sistemi | Ubuntu 22.04 (Jammy) |
| ROS2 dağıtımı | Humble Hawksbill |
| Simülasyon | Gazebo Harmonic (`ros-humble-ros-gzharmonic`) |
| BT kütüphanesi | BehaviorTree.CPP 4.9.1 (yalnızca; py_trees kullanılmaz) |
| Programlama dili | C++17 (analiz scriptleri hariç → Python) |
| Görselleştirme | Groot2 |
| Loglama | rosbag2 |
| Lisans | Yok / private |

## Repo Yapısı

```
.                              ← colcon workspace kökü (= plandaki swarm_bt_ws/)
├── src/
│   ├── swarm_bt_bringup/      ← launch dosyaları + YAML deney config'leri
│   │   ├── launch/
│   │   └── config/
│   ├── swarm_bt_core/         ← BT node'ları, negotiation subtree, BT.CPP entegrasyonu
│   │   └── bt_xml/            ← bt_central.xml, bt_distributed.xml, bt_event_driven.xml
│   ├── swarm_bt_msgs/         ← özel mesaj/servis tanımları
│   ├── swarm_bt_sim/          ← Faz 1 hafif kinematik simülatör
│   └── swarm_bt_analysis/     ← metrik hesaplama ve görselleştirme (Python)
├── experiments/               ← deney kayıt CSV/DB
└── README.md
```

## Kurulum ve Derleme

Ön koşullar (bu makinede kurulu ve doğrulandı): ROS2 Humble, Gazebo Harmonic
(`ros-humble-ros-gzharmonic`), `ros-humble-behaviortree-cpp` (4.9.1), PX4-Autopilot.

```bash
source /opt/ros/humble/setup.bash
colcon build --symlink-install
colcon test && colcon test-result --all
```

BT.CPP prefix overlay'i (bkz. Varsayımlar V2) ilk `colcon build` sırasında CMake
otomatik üretir; elle üretmek için `./tools/setup_btcpp_overlay.sh`.

Altyapının hangi parçasının hazır olduğunu görmek için:

```bash
./tools/check_infra.sh
```

> **Not:** Başarısız bir derlemeden sonra CMake cache'i bozulabiliyor
> (`install/<paket>/share/.../package.sh bulunamadı` hatası). Çözüm:
> `rm -rf build install log && colcon build --symlink-install`.

## Varsayımlar

Plan dokümanı, belirsiz teknik kararlarda durup sormak yerine makul bir varsayım
seçip not düşmeyi şart koşuyor. Alınan kararlar burada toplanır.

### V1 — Repo kökü doğrudan colcon workspace kökü
Plandaki iskelet `swarm_bt_ws/` adlı bir üst klasör gösteriyor. Repo klasörü
(`bt-imp-in-swarm`) bu rolü doğrudan üstleniyor; `src/`, `experiments/` ve
`README.md` repo kökünde. Gerekçe: `bt-imp-in-swarm/swarm_bt_ws/src/...` şeklinde
gereksiz bir iç içe geçme katmanı oluşmasın. İşlevsel fark yok — `colcon build`
repo kökünden çalışır.

### V2 — BT.CPP apt paketinde kütüphane yolu hatası, overlay ile çözüldü
`ros-humble-behaviortree-cpp` 4.9.1 paketinde kütüphane
`/opt/ros/humble/lib/x86_64-linux-gnu/libbehaviortree_cpp.so` altına kuruluyor,
ancak paketin `ament_cmake_export_libraries` shim'i onu `/opt/ros/humble/lib`
içinde arıyor (`NO_DEFAULT_PATH` kullandığı için mimariye özgü alt dizine
bakmıyor). Sonuç: `find_package(behaviortree_cpp)` doğrudan hata veriyor.

Çözüm: repo içinde `sudo` gerektirmeyen, sembolik linklerden oluşan bir prefix
overlay (`tools/setup_btcpp_overlay.sh` ile üretilir, git'e girmez). Ayrıntı ve
alternatif tek-satırlık `sudo` çözümü Faz 0 kurulum bölümünde.

CMake target adı `behaviortree_cpp::behaviortree_cpp` (bazı dokümanlarda geçen
`BT::behaviortree_cpp` bu pakette tanımlı değil).

### V3 — Git kimliği ve push politikası
Commit'ler `mericesra01@gmail.com` kimliğiyle ve **yalnızca yerelde** atılır.
Plan gereği hiçbir aşamada `git push` çalıştırılmaz; push'u kullanıcı yapar.

### V4 — `<license>` tag'i "Proprietary"
Plan "Lisans: Yok / private" diyor, ancak ROS2 (`catkin_pkg`) boş bir `<license>`
tag'ini reddediyor ve paket derlenmiyor. Beş paketin tamamında
`<license>Proprietary</license>` kullanıldı — bu, ROS2 ekosisteminde "açık kaynak
lisansı yok, özel" anlamına gelen standart ifade. Repoya `LICENSE` dosyası
eklenmedi.

### V5 — cpplint'in copyright kuralı kapalı
Lisans dosyası olmadığı için `ament_cpplint` her kaynak dosyayı "No copyright
message found" ile işaretliyordu. C++ paketlerine `CPPLINT.cfg`
(`filter=-legal/copyright`) eklendi; cpplint'in geri kalan kuralları (header
guard, include sırası, satır uzunluğu) ve uncrustify/cppcheck/lint_cmake/xmllint
tam olarak açık. Faz 0 sonunda tüm lint + test paketi temiz:
**36 test, 0 hata, 0 başarısızlık**.

### V6 — Şerit atamasında sürü düzeyinde serpantin desen
İlk uygulamada alan N eşit dikey şeride bölünüp hepsi aynı yönde (soldan sağa)
taranıyordu. Bu, ajanları **sabit sütun farkıyla kilitli** ilerletiyor ve
sonuçta hiçbir drone diğerine yaklaşmıyordu: 400 m'lik alanda r_comm=60 m ile
**bir koşuda sıfır karşılaşma olayı** üretiliyordu. Karşılaşmalar bu çalışmanın
tüm dayanağı olduğu için (Bölüm 2.2 negotiation, Bölüm 6 churn/karşılaşma
sıklığı metrikleri) model bu haliyle kullanılamazdı.

Çözüm: bitişik ajanlara zıt sütun yönü verildi (ajan 0 soldan sağa, ajan 1
sağdan sola, ...). Bu, sürü düzeyinde serpantin bir desen oluşturur; komşu
ajanlar ortak sınırlarında buluşur. Sonuç: N=3 → 2 karşılaşma, N=5 → 4
karşılaşma (yaklaşık N-1 sınır çifti), N ile monoton artıyor.

Karşılaşma sayısı hâlâ düşük; bu bilinçli olarak Bölüm 1'deki `r_comm`
kalibrasyonuna bırakıldı (plan: "karşılaşmaların çok seyrek ya da çok sık
olmaması için" kalibre edilecek).

### V7 — Simülasyon zamanı tick sayısından türetilir
`time += dt` biriktirmesi kayan nokta kayması yaratıyor (10 × 0.1 = 0.999...)
ve zaman sınırı kontrolünü bir tick kaydırıyordu. Simülatör artık zamanı
`tick_count * dt` olarak yazıyor.

### V8 — Ajan başına tohumlanmış hız sapması eklendi
`r_comm` kalibrasyonu (Bölüm 1) modelin dejenere olduğunu ortaya çıkardı:
tüm ajanlar tam olarak aynı hızda uçtuğu için şeritler boyunca **rijit
formasyonda** kilitleniyor, `r_comm` 20 m'den 140 m'ye çıkarılsa bile karşılaşma
sayısı hiç değişmiyordu. Ajan başına ±%5 hız sapması (`speed_jitter`,
tohumlanmış) eklendi. Gerçek dronelar da tam olarak aynı hızda uçmaz; ayrıca
tohumlanmış olması planın istediği ≥10 tekrarın tekrarlanabilirliğini sağlar.

### V9 — Karşılaşma tespitine histerezis eklendi
Biçerdöver deseninde bitişik sütunlarda paralel süpüren iki drone arasındaki
mesafe **tam olarak** hücre boyutunun katıdır ve hız sapmasıyla eşiğin iki
yanında salınır. Histerezissiz eşik, tek bir yakınlaşmayı 130 sahte
"karşılaşma"ya bölüyordu — Bölüm 6'nın karşılaşma sıklığı ve churn oranı
metriklerini tamamen bozacak bir hata. Giriş eşiği `r_comm`, çıkış eşiği
`r_comm × 1.1` yapıldı. Ayrıntı: [`experiments/calibration_rcomm.md`](experiments/calibration_rcomm.md).

### V10 — Workspace dışından tüketim overlay gerektirir
`swarm_bt_core`, `behaviortree_cpp`'yi `ament_export_dependencies` ile dışa
aktardığı için, bu paketleri workspace dışından kullanan bir CMake projesi de
V2'deki overlay'e ihtiyaç duyar:

```bash
export CMAKE_PREFIX_PATH="<repo>/.btcpp_overlay:$CMAKE_PREFIX_PATH"
```

Workspace içinde çalışırken bu gerekmez (her paket `cmake/btcpp_prefix_fix.cmake`
üzerinden kendisi hallediyor).

### V11 — Takas kabul ölçütünün iyi tanımlı hali
Plan: *"teklif alan taraf, takas sonrası toplam kat edeceği mesafenin azalıp
azalmadığına bakar"*. Hücreleri **devralan** taraf her zaman iş üstlenir,
dolayısıyla kendi kat edeceği mesafe tek başına asla azalmaz — ölçüt bu haliyle
hiçbir takası kabul etmezdi.

İyi tanımlı hali: alıcı, hücreleri üstlenmenin kendi turuna ekleyeceği mesafeyi
(`receiver_cost`), teklif edenin bu hücrelerden kurtularak kazanacağı mesafeyle
(`proposer_gain`) karşılaştırır ve **`receiver_cost < proposer_gain`** ise kabul
eder — yani takas **ikilinin toplam** kat edeceği mesafeyi azaltıyorsa. Bu,
"toplam mesafe azalıyor mu" okumasına sadıktır, iyi tanımlıdır ve takasın
geometrik olarak anlamlı olmasını sağlar: hücreler ancak onları daha ucuza
kapatabilecek ajana geçer.

Teklif edilen hücreler, teklif edenin kalan hücrelerinden **alıcıya en yakın**
olanlardır ve sayıları ikiliyi dengeleyecek kadardır (farkın yarısı).

### V12 — Dağıtık devralma tek başına kilitleniyor, boşta kalan ajan gerekiyor
Plan Bölüm 2.3 arızalanan drone'un alanının "nasıl devralındığını" gözlemlemeyi
istiyor. Devralmayı yalnızca karşılaşma anına bağlamak **çalışmıyor**: N=3'te
ortadaki drone arızalanırsa hayatta kalan iki ajan ~140 m arayla kalır ve
`r_comm = 60 m` ile birbirlerini **hiç görmezler** — sahipsiz alan sonsuza kadar
sahipsiz kalır, görev zaman sınırına dayanır.

Eklenen ikinci yol: kendi bölgesini bitirip **boşta kalan** ajan, stigmerji
haritasındaki taranmamış hücreleri görüp üstlenir (`claimOrphansIfIdle`).
Merkezi bir dağıtıcı gerektirmez, dolayısıyla tam dağıtık mimariyle (P2c)
uyumludur. Ölçülen: arıza t=132 s'de, 73 hücre devralınıyor, görev 278 s yerine
417 s'de tamamlanıyor.

Devralma **sonrası** oluşan büyük iş yükü dengesizliği (ajan 2: 4181 m, ajan 0:
2780 m) takas müzakeresiyle düzeltilemiyor, çünkü iki ajan yine buluşmuyor.
Bu, P2 (koordinasyon mimarisi) karşılaştırmasının ölçmesi beklenen etkilerden
biri: merkezi/hiyerarşik mimarilerde bu bilgi buluşma gerektirmeden akar.

### V13 — Rastgele kalkış konumları: P3 ekseninin anlamlı olmasının koşulu
İlk uygulamada her drone doğrudan kendi şeridinin ilk waypoint'ine konuyordu.
O durumda alan atama problemi **önceden çözülmüş** oluyor: ihaleli algoritmalar
(P3b Contract Net, P3c CBBA) eşit şerit bölmesini **birebir yeniden üretiyor**
ve P3 ekseni OFAT taramasında hiçbir şey ölçmüyordu (test bunu yakaladı).

Dronelar artık görev alanı içinde tohumlanmış rastgele konumlardan kalkıyor
(`sim.random_launch`, varsayılan açık). Bu iki sorunu birden çözüyor:

1. **P3 ayrışıyor**: P3a şeritleri ajan indeksine göre dağıtır (konum-kör),
   P3b/P3c tekliflerini gerçek konumlara göre verir. Ölçülen (N=3, 10 tekrar
   ortalaması): P3a 322.7 s, P3b 311.2 s, P3c 313.7 s.
2. **Tekrarlar anlam kazanıyor**: önceden koşular arası tek rastgelelik hız
   sapmasıydı ve görev süresi neredeyse sabitti. Artık 295-340 s arasında
   gerçek değişkenlik var — planın istediği ≥10 tekrar bir şey ortalıyor.

### V14 — Churn oranı yalnızca karşılaşma kaynaklı değişiklikleri sayar
Boşta kalan ajanın sahipsiz alanı üstlenmesi (V12) bir karşılaşma sonucu
değildir. Churn oranının payına dahil edilince oran 1'i aşabiliyordu (test
yakaladı: 1 churn / 0 karşılaşma). Ayrı `idle_claims` sayacına alındı.

### V15 — Stigmerjinin (P5b) görev süresine etkisi bu modelde bağlamıyor
Bölgeler kesin partisyon olduğu için (her hücre tam olarak bir ajana ait, takas
ve devralma da bunu korur) **mükerrer tarama hiç oluşmuyor**. Dolayısıyla
stigmerjinin "başkasının taradığı hücreyi atla" faydası görev süresini
değiştirmiyor — ölçüldü: `P5abc` ve `P5ac` birebir aynı süreyi veriyor.

Stigmerjinin bu modelde bağladığı yer ajanların **bilgi durumu**: açıkken her
ajan taranmış tüm hücreleri bilir (`known_coverage_ratio = 1.0`), kapalıyken
yalnızca kendi taradıklarını ve karşılaşmalarda öğrendiklerini bilir. Bu ayrı
bir metrik olarak ölçülüyor.

Görev süresine etkisinin görüneceği yer Bölüm 2.2'nin **ortak tarama**
(`joint_scan_baslat`) dalı: iki drone sınırda kasıtlı olarak örtüşen bir alanı
tarar, ve orada stigmerji mükerrerliği önler. O dal negotiation alt-ağacıyla
(Bölüm 5/Faz 0) birlikte geliyor; P5b'nin süre etkisi orada yeniden ölçülecek.

### V16 — Hiyerarşik hibrit (P2b), N=3-5 ölçeğinde tam dağıtıktan ayırt edilemiyor
P2b'nin P2c'den farkı, bir kümenin **bütün olarak** planlanmasıdır: lider,
kümedeki tüm üyeleri görüp ardışık dengeleme adımları uygular. İkili bir kümede
lider, ikili pazarlığın vereceği kararı verir — fark ancak **üç ve daha fazla**
üyeli kümede ortaya çıkar.

Ölçüldü: N=3 ve N=5'te, `r_comm = 60 m` ile üç drone aynı anda karşılıklı
menzilde **neredeyse hiç** bulunmuyor. 20 koşuda oluşan üç-üyeli küme sayısı
≤2. Sonuç: P2b ve P2c **birebir aynı** görev süresini veriyor (test bunu
belgeliyor). P2a (tam merkezi) ise ikisinden de ayrışıyor.

Bu, araştırma sorusu 2 için doğrudan bir bulgu: hiyerarşik koordinasyonun
ayrışması için sürünün, aynı anda üç ajanı bir arada tutacak kadar **yoğun**
olması gerekiyor. N=3→N=5 bu eşiği geçmiyor.
