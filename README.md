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
