# Deney Kayıtları

Bu dizin, çalışmanın tüm ölçüm çıktılarını ve analiz raporlarını tutar.
Her rapor, hangi plan maddesine karşılık geldiğini başında belirtir.

## Ortak veritabanı

| dosya | içerik |
|---|---|
| [`deney_veritabani.csv`](deney_veritabani.csv) | tüm koşuların ortak tablosu (52 satır) |
| [`deney_veritabani_semasi.md`](deney_veritabani_semasi.md) | şema, anahtar ve heterojen çalışmanın nasıl ekleneceği (Bölüm 8) |

## Kalibrasyonlar

| dosya | plan maddesi |
|---|---|
| [`calibration_rcomm.md`](calibration_rcomm.md) · [`.csv`](calibration_rcomm.csv) | Bölüm 1 — `r_comm` ilk kalibrasyonu |
| [`calibration_rcomm_bt.md`](calibration_rcomm_bt.md) · [`.csv`](calibration_rcomm_bt.csv) | Bölüm 5/Faz 0 — tam BT modeliyle yeniden kalibrasyon |
| [`calibration_threshold.md`](calibration_threshold.md) · [`.csv`](calibration_threshold.csv) | Bölüm 2.2 — `esik_degeri` ilk kalibrasyonu |
| [`calibration_threshold_bt.md`](calibration_threshold_bt.md) · [`.csv`](calibration_threshold_bt.csv) | Bölüm 5/Faz 0 — yeniden kalibrasyon (0.20 → 0.10) |
| [`esik_hassasiyeti.md`](esik_hassasiyeti.md) | Bölüm 9 — eşik riskinin iki ucu |

## Faz 1 — kod-seviyesi tarama

| dosya | içerik |
|---|---|
| [`ofat_phase1.csv`](ofat_phase1.csv) | 16 kombinasyon × 2 ölçek × 10 tekrar = 320 koşu |
| [`ofat_phase1_ozet.md`](ofat_phase1_ozet.md) | manşet bulgular |
| [`ofat_phase1_rapor.md`](ofat_phase1_rapor.md) | yön çatışmaları, ölçek eşikleri, N-duyarlılık skorları |
| [`metrik_tablosu_faz1.csv`](metrik_tablosu_faz1.csv) · [`.md`](metrik_tablosu_faz1.md) | N=3→N=5 delta tablosu (Bölüm 5/Faz 1) |

## Faz 2 — Gazebo doğrulaması

| dosya | içerik |
|---|---|
| [`phase2_gazebo.csv`](phase2_gazebo.csv) | 5 finalist × 2 ölçek × 5 tekrar × 2 faz = 100 koşu |
| [`phase2_ozet.md`](phase2_ozet.md) | model doğruluğu, ölçek artışı, yön uyumu |
| [`phase2_karsilastirma.md`](phase2_karsilastirma.md) | kod-seviyesi ↔ Gazebo karşılaştırması |

## Faz 3 — analiz

| dosya | plan maddesi |
|---|---|
| [`figures/`](figures/) | N=3→N=5 slope chart'ları + duyarlılık sıralaması |
| [`manipulasyona_aciklik.md`](manipulasyona_aciklik.md) | araştırma sorusu 2'nin cevabı |
| [`parametre_duyarliligi.md`](parametre_duyarliligi.md) | eksen sıralaması, merkezi mi dağıtık mı |

## Faz 4 ve Bölüm 9

| dosya | içerik |
|---|---|
| [`faz4_heterojen_karsilastirma.md`](faz4_heterojen_karsilastirma.md) | heterojen çalışmayla karşılaştırma protokolü ve hipotezler |
| [`confound_kontrol.csv`](confound_kontrol.csv) · [`confound_kontrol.md`](confound_kontrol.md) | karşılaşma sıklığı sabitlenerek N'in saf etkisi |
| [`olcek_genisletme.csv`](olcek_genisletme.csv) · [`olcek_genisletme.md`](olcek_genisletme.md) | orantılı alan ve N=7/N=10 |

## Yeniden üretme

```bash
source /opt/ros/humble/setup.bash && source install/setup.bash

# Kalibrasyonlar
ros2 run swarm_bt_sim calibrate_rcomm --repetitions 10 > experiments/calibration_rcomm_bt.csv
ros2 run swarm_bt_sim calibrate_threshold --repetitions 6 > experiments/calibration_threshold_bt.csv

# Faz 1
ros2 run swarm_bt_sim ofat_sweep --repetitions 10 --phase faz1 > experiments/ofat_phase1.csv
ros2 run swarm_bt_analysis ofat_report experiments/ofat_phase1.csv -o experiments/ofat_phase1_rapor.md
ros2 run swarm_bt_analysis metrics_table experiments/ofat_phase1.csv \
    -o experiments/metrik_tablosu_faz1.csv -m experiments/metrik_tablosu_faz1.md

# Faz 2 (Gazebo; ~45 dakika)
./tools/run_phase2.sh 5 experiments/phase2_gazebo.csv
ros2 run swarm_bt_analysis phase_comparison experiments/phase2_gazebo.csv \
    -o experiments/phase2_karsilastirma.md

# Faz 3
ros2 run swarm_bt_analysis make_figures experiments/ofat_phase1.csv -o experiments/figures
ros2 run swarm_bt_analysis stability_hypothesis experiments/ofat_phase1.csv \
    -o experiments/manipulasyona_aciklik.md
ros2 run swarm_bt_analysis parameter_sensitivity experiments/ofat_phase1.csv \
    -o experiments/parametre_duyarliligi.md

# Bölüm 9 kontrol deneyleri
ros2 run swarm_bt_sim confound_control --repetitions 10 > experiments/confound_kontrol.csv
ros2 run swarm_bt_sim scale_extension --repetitions 10 > experiments/olcek_genisletme.csv

# Ortak veritabanı
ros2 run swarm_bt_analysis experiment_db experiments/ofat_phase1.csv \
    -d experiments/deney_veritabani.csv
```
