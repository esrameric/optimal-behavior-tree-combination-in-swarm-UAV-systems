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
