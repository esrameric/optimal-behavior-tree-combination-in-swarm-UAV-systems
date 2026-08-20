# `esik_degeri` Hassasiyet Analizi — Bölüm 9 Riski

**Plan Bölüm 9:** *"`esik_degeri` çok düşükse sistem sürekli takas yapar
(gürültü), çok yüksekse hiç takas olmaz — Faz 0'da bu değeri belirlerken
birkaç ön-deneme yap."*

Ön-denemeler yapıldı ve **iki uç da ölçüldü**. Veri:
[`calibration_threshold_bt.csv`](calibration_threshold_bt.csv) (arızalı senaryo,
6 tekrar). Seçilen değer ve gerekçesi:
[`calibration_threshold_bt.md`](calibration_threshold_bt.md).

## Riskin iki ucu, ölçülmüş hâliyle

### Alt uç — "sürekli takas yapar (gürültü)"

Planın öngördüğü gürültü **takas sayısında değil, TEKLİF sayısında** ortaya
çıkıyor. N=5, eşik 0'da:

- **5.67 teklif/koşu** kurulur, ama yalnızca **0.33'ü kabul edilir**
- Yani tekliflerin **%94'ü boşa gider**

Boşa giden her teklif bir müzakere turudur: durum bilgisi alışverişi, tur
mesafesi hesabı, mesaj trafiği. Görev süresine katkısı yok, iletişim yüküne
katkısı var. **Gürültü buradadır.**

Kabul edilen takas sayısının sabit kalması, fayda ölçütünün (Bölüm 2.2)
görevini yaptığını gösterir: eşik gevşese de anlamsız takaslar reddedilir.
Yani düşük eşik sistemi *bozmuyor*, **israf ettiriyor**.

### Üst uç — "hiç takas olmaz"

N=5, eşik ≥ 0.15'te takas etkinliği **yarıya düşüyor** (0.333 → 0.167) ve görev
süresi **%3.6 kötüleşiyor** (288.1 → 298.5 s). Eşik ≥ 0.35'te teklif sayısı da
sıfıra iner.

### N=5, arızalı senaryo (6 tekrar)

| esik_degeri | teklif | takas | görev süresi (s) | atama kararlılığı |
|---|---|---|---|---|
| 0.00 | 5.67 | 0.333 | 288.1 | 0.600 |
| 0.05 | 4.67 | 0.333 | 288.1 | 0.600 |
| 0.10 | 2.67 | 0.333 | 288.1 | 0.600 |
| 0.15 | 1.33 | 0.167 | 298.5 | 0.533 |
| 0.20 | 1.33 | 0.167 | 298.5 | 0.533 |
| 0.25 | 0.67 | 0.167 | 298.5 | 0.533 |
| 0.30 | 0.33 | 0.167 | 298.5 | 0.533 |
| 0.35 | 0.33 | 0.167 | 298.5 | 0.533 |

## Seçim: %10 — israf ile etkinlik arasındaki verimlilik sınırı

| | eşik 0.00 | **eşik 0.10** | eşik 0.15 |
|---|---|---|---|
| teklif/koşu | 5.67 | **2.67** | 1.33 |
| kabul edilen takas | 0.333 | **0.333** | 0.167 |
| görev süresi | 288.1 s | **288.1 s** | 298.5 s |
| boşa giden teklif oranı | %94 | **%88** | %87 |

%10, **aynı performansı yarıdan az müzakere yüküyle** veren en yüksek eşiktir.
Bir adım yukarısı (%15) takas etkinliğini yarıya düşürüyor.

## Üçüncü bir risk — ölçüm sırasında ortaya çıktı

Planın öngörmediği bir uç daha var: **N=3'te hiçbir eşik değerinde takas kabul
edilmiyor.**

| esik_degeri | teklif (N=3) | takas (N=3) |
|---|---|---|
| 0.00 | 2.00 | 0.000 | 467.8 | 0.667 |
| 0.05 | 1.17 | 0.000 | 467.8 | 0.667 |
| 0.10 | 0.33 | 0.000 | 467.8 | 0.667 |
| 0.15 | 0.17 | 0.000 | 467.8 | 0.667 |
| 0.20 | 0.17 | 0.000 | 467.8 | 0.667 |
| 0.25 | 0.17 | 0.000 | 467.8 | 0.667 |

Üç drone ile alan yeterince büyük paylara düşüyor ve dengesizlik, sütun devrini
haklı çıkaracak düzeye çıkmıyor. N=3'te yeniden-atama **tamamen arıza
devralmasından** geliyor.

Sonuç: `esik_degeri`'nin kalibrasyonu **ölçeğe bağlıdır.** N=3'te parametre
ölüdür; N=5'te çalışır ve %10-15 arasında keskin bir eşik gösterir. Tek bir
değerin iki ölçeğe birden hizmet etmesi bu çalışmada mümkün oldu, ama daha
büyük N'lerde ölçek başına ayrı kalibrasyon gerekebilir.
