# `r_comm` Yeniden Kalibrasyonu — Bölüm 5/Faz 0

**Tarih:** 2026-08-20 · **Araç:** `ros2 run swarm_bt_sim calibrate_rcomm --repetitions 10`
**Ham veri:** [`calibration_rcomm_bt.csv`](calibration_rcomm_bt.csv)
**Önceki kesit:** [`calibration_rcomm.md`](calibration_rcomm.md) (BT karar katmanı öncesi)

## Karar: `r_comm = 60 m` **değişmedi**

Bölüm 1'deki seçim, tam BT modeliyle yapılan bu ikinci taramada da geçerli
çıktı. 400 m'lik alanın **%15'i**, planın önerdiği %10-20 bandının ortası.

| N | karşılaşma (ort ± std) | görev süresi (ort ± std) | tamamlanan |
|---|---|---|---|
| 3 | 2.4 ± 1.4 | 468.7 ± 27.4 s | 10/10 |
| 5 | 7.4 ± 1.8 | 302.4 ± 45.3 s | 10/10 |

## İlk taramaya göre ne değişti

**Eğri artık düzgün ve monoton.** Bölüm 1'deki taramada karşılaşma sayısı
`r_comm`'a ya hiç tepki vermiyor ya da 40 m periyotlu sıçramalar gösteriyordu.
Şimdi menzil arttıkça karşılaşma sayısı düzenli artıyor:

| r_comm | %  | karşılaşma N=3 | karşılaşma N=5 |
|---|---|---|---|
| 20 m | 5 | 0.5 | 2.3 |
| 40 m | 10 | 1.5 | 4.5 |
| **60 m** | **15** | **2.4** | **7.4** |
| 80 m | 20 | 2.8 | 9.9 |
| 100 m | 25 | 3.8 | 11.3 |

Farkın sebebi rastgele kalkış konumlarının (V13) ve BT karar katmanının
getirdiği çeşitlilik: ajanlar artık rijit bir desende ilerlemiyor.

**N=5, N=3'ün yaklaşık 3 katı karşılaşma üretiyor** (7.4 vs 2.4) — alan sabit
tutulduğu için (Bölüm 1) beklenen yoğunluk etkisi. Bu, Bölüm 9'da raporlanması
istenen confound'un sayısal büyüklüğüdür.

## Görev süresi `r_comm`'a duyarsız

Görev süresi tüm menzil aralığında N=3 için ~460-478 s, N=5 için ~276-308 s
bandında kalıyor. `r_comm` performans metriğini doğrudan sürüklemiyor; yalnızca
koordinasyon fırsatlarının sayısını belirliyor. Bu iyi bir özellik: menzil
seçimi, ölçek karşılaştırmasını confound etmiyor.
