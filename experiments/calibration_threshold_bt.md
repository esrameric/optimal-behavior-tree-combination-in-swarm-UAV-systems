# `esik_degeri` Yeniden Kalibrasyonu — Bölüm 5/Faz 0

**Tarih:** 2026-08-20 · **Araç:** `ros2 run swarm_bt_sim calibrate_threshold --repetitions 6`
**Ham veri:** [`calibration_threshold_bt.csv`](calibration_threshold_bt.csv)
**Önceki kesit:** [`calibration_threshold.md`](calibration_threshold.md) (BT öncesi)

## Karar: `esik_degeri` **0.20 → 0.10**

| | plan önerisi | Bölüm 2.2 (BT öncesi) | **Bölüm 5/Faz 0 (BT ile)** |
|---|---|---|---|
| `esik_degeri` | 0.30 | 0.20 | **0.10** |

Arızalı senaryo, N=5, 6 tekrar:

| esik_degeri | teklif | takas | churn | atama kararlılığı | **görev süresi** |
|---|---|---|---|---|---|
| 0.00 | 5.67 | 0.333 | 0.052 | 0.600 | 288.1 s |
| 0.05 | 4.67 | 0.333 | 0.052 | 0.600 | 288.1 s |
| **0.10** | **2.67** | **0.333** | **0.052** | **0.600** | **288.1 s** |
| 0.15 | 1.33 | 0.167 | 0.033 | 0.533 | 298.5 s ⚠ |
| ≥0.20 | ≤1.33 | 0.167 | 0.033 | 0.533 | 298.5 s |

%15 ve üzerinde takas etkinliği **yarıya düşüyor** (0.333 → 0.167) ve görev
süresi %3.6 kötüleşiyor. %10, aynı sonucu veren en yüksek eşik: %0'a göre
teklif sayısını **iki kattan fazla** azaltıyor (5.67 → 2.67), yani aynı
performansı yarıdan az müzakere yüküyle elde ediyor.

## Neden ilk kalibrasyondan farklı çıktı

İki model değişikliği:

1. **BT karar katmanı**: karar artık negotiation alt-ağacının Fallback
   sırasından geçiyor; takas dalı reddedilirse ortak tarama ve bilgi paylaşımı
   dalları devreye giriyor.
2. **Takas granülerliği hücre → sütun** (aşağıya bakınız).

## Kritik düzeltme — takas granülerliği

BT entegrasyonundan sonra yapılan ilk ölçümde **hiçbir eşikte tek bir takas
bile kabul edilmiyordu**. Sebep geometrik: takas tek tek hücre devrediyordu ve
biçerdöver taramada birkaç hücreyi komşuya vermek

- **alıcıyı** yeni bir sütuna sapmaya zorluyor (yüksek maliyet),
- **teklif edeni** ise zaten geçeceği yoldan neredeyse hiç kurtarmıyor.

Ölçülen ortalama fayda: **−333 m**. Fayda ölçütü bu anlamsız takasları doğru
şekilde reddediyordu; hatalı olan devir birimiydi.

Devir birimi **tam sütuna** çevrildi. Teklif eden artık o sütuna hiç uğramıyor,
kazanç gerçek oluyor. P3 tahsis algoritmaları da zaten sütun granülerliğinde
çalışıyor, dolayısıyla tasarım bütünlüğü de sağlanmış oldu. Sonuç: N=5'te
koşu başına 0.33 kabul edilen takas.

## Ölçek bulgusu — eşik yalnızca N=5'te bağlıyor

**N=3'te hiçbir eşikte takas kabul edilmiyor.** Üç drone ile alan yeterince
büyük paylarına düşüyor ve dengesizlik, sütun devrini haklı çıkaracak düzeye
çıkmıyor. Yeniden-atama N=3'te neredeyse tamamen arıza devralmasından geliyor.

Bu, araştırma sorusu 2 için doğrudan bir bulgu: **takas mekanizması ölçekle
birlikte devreye giriyor.** N=3'te ölü bir dal, N=5'te çalışan bir mekanizma.
