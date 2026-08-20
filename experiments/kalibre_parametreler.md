# Kalibre Edilmiş Parametreler — Bölüm 10

Bu belge, çalışmanın **başlangıç değerlerinin tek referansıdır**. Değerler
`ExperimentConfig` varsayılanlarıyla birebir aynıdır ve bir test bunu
doğrular (`test_calibrated_defaults.cpp`) — belge koddan kayamaz.

## Değerler

| parametre | değer | nerede belirlendi | gerekçe |
|---|---|---|---|
| **`r_comm`** | **60 m** | Bölüm 1, Bölüm 5/Faz 0'da doğrulandı | 400 m alanın %15'i; planın önerdiği %10-20 bandının ortası. Karşılaşma sayısının menzile gerçekten tepki verdiği bandın içinde. |
| **`esik_degeri`** | **0.10** | Bölüm 2.2, Bölüm 5/Faz 0'da **revize edildi** | Planın önerdiği %30 çalışma bandının dışında; %20 (ilk kalibrasyon) BT modeliyle fazla yüksek çıktı. %10, aynı performansı yarıdan az müzakere yüküyle veren en yüksek eşik. |
| `encounter_hysteresis` | 0.10 | Bölüm 1 | Histerezissiz eşik, tek bir yakınlaşmayı 130 sahte karşılaşmaya bölüyordu. |
| `speed_jitter` | 0.05 | Bölüm 1 | Sıfır olursa ajanlar rijit formasyonda kilitleniyor ve `r_comm` hiçbir etki göstermiyor. |
| `joint_scan_threshold` | 0.25 | Bölüm 5/Faz 0 | Sınır bölgesindeki ortalama feromon eşiği. |
| `interest_points` | 12 | Bölüm 5/Faz 0 | Feromonun "ortak ilgi" anlamı taşıması için gereken kaynak. |
| `safety_radius` | 5 m | Bölüm 4 | Çarpışma (yakınlık ihlali) metriğinin eşiği. |
| mission alanı | 400 × 400 m, hücre 20 m | Bölüm 1 | **N'den bağımsız sabit** — karşılaştırmanın dayanağı. |

## Nasıl değiştirilir

Değerler üç yerde birden görünür ve **üçü de senkron tutulmalıdır**:

1. `swarm_bt_core/include/swarm_bt_core/experiment_config.hpp` — varsayılanlar
2. `swarm_bt_bringup/config/experiment_template.yaml` — şablon
3. bu belge

`test_calibrated_defaults.cpp` (1) ile (3) arasındaki tutarlılığı,
`test_config_files.cpp` ise (1) ile (2) arasındakini doğrular. Bir değeri
değiştirip testleri koşturmadan bırakmak mümkün değildir.

## Yeniden kalibrasyon

Model değiştiğinde (yeni bir mekanizma, farklı bir senaryo) kalibrasyonlar
yeniden koşulmalı:

```bash
ros2 run swarm_bt_sim calibrate_rcomm --repetitions 10 > experiments/calibration_rcomm_bt.csv
ros2 run swarm_bt_sim calibrate_threshold --repetitions 6 > experiments/calibration_threshold_bt.csv
```

Bu çalışmada iki kez kalibre edildi ve **ikinci turda `esik_degeri` değişti**
(0.20 → 0.10) — kalibrasyonun modele bağımlı olduğunun somut kanıtı.
