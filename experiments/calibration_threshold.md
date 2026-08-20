# `esik_degeri` Kalibrasyonu — Bölüm 2.2

**Tarih:** 2026-08-20
**Araç:** `ros2 run swarm_bt_sim calibrate_threshold --repetitions 10`
**Ham veri:** [`calibration_threshold.csv`](calibration_threshold.csv)

## Karar

| Parametre | Plan önerisi | **Seçilen** | Gerekçe |
|---|---|---|---|
| `esik_degeri` | %30 | **%20** | %30 çalışma bandının **dışında** kalıyor (aşağıya bakınız) |

## Bulgu 1 — Arızasız baseline'da eşiğin hiçbir etkisi yok

Eşit şeritli baseline'da (P3a, arıza yok) ajanlar aynı oranda ilerler; kalan
alan oranları arasındaki fark **hiçbir zaman %5'i aşmaz**. Eşik %5'in üstündeki
her değerde sıfır teklif doğar:

| esik_degeri | teklif (N=3) | takas | görev süresi |
|---|---|---|---|
| 0.00 | 2.2 | 0 | 290.61 s |
| 0.05 – 0.90 | **0.0** | 0 | 290.61 s |

Eşik 0'da bile takas kabul edilmiyor: kurulan tekliflerin ortalama faydası
**−333 m** (alıcının maliyeti, teklif edenin kazancından bu kadar fazla).
Sebep geometrik — tek tük hücrenin komşuya devri, alıcıyı yeni bir sütuna
sapmaya zorlarken teklif edenin zaten geçeceği yoldan neredeyse hiçbir şey
eksiltmez. Fayda ölçütü bu anlamsız takasları doğru şekilde reddediyor.

**Sonuç:** eşiği kalibre etmek için dengesizlik üreten bir senaryo şart.
Bu senaryo arızadır (Bölüm 2.3).

## Bulgu 2 — Arızalı senaryoda net bir uçurum var, %30 onun ötesinde

Arıza açıkken (görev ortasında rastgele bir drone), N=3:

| esik_degeri | teklif | takas | atama kararlılığı | **görev süresi** |
|---|---|---|---|---|
| 0.00 | 2.9 | 0.2 | 0.833 | 425.64 s |
| 0.05 | 1.7 | 0.2 | 0.833 | 425.64 s |
| 0.10 | 1.2 | 0.2 | 0.833 | 425.64 s |
| 0.15 | 0.9 | 0.2 | 0.833 | 425.64 s |
| **0.20** | **0.9** | **0.2** | **0.833** | **425.64 s** |
| 0.25 | 0.9 | 0.2 | 0.833 | 425.64 s |
| 0.30 | 0.3 | **0.0** | 0.700 | **444.67 s** ⚠ |
| ≥0.35 | 0.0 | 0.0 | 0.700 | 444.67 s |

**Planın önerdiği %30, takas mekanizmasının kapandığı ilk değer.** Orada görev
süresi %4.5 kötüleşiyor (425.64 → 444.67 s) ve atama kararlılığı düşüyor.

Çalışma bandı **%5 – %25**. Bandın içinde görev süresi ve takas sayısı sabit;
tek değişen, boşuna kurulup reddedilen teklif sayısı — yani iletişim yükü
(%0'da 2.9 teklif, %20'de 0.9 teklif, aynı sonuç için 3× daha az müzakere).

**Seçim: %20.** Bandın üst ucuna yakın (düşük müzakere yükü) ama uçurumdan
bir adım geride (%25 tam sınırda kalırdı).

## Bulgu 3 — N=5 hiçbir eşikte takas yapmıyor (erken ölçek sinyali)

| N | devralınan hücre | takas | churn oranı | atama kararlılığı |
|---|---|---|---|---|
| 3 | 66.6 | 0.2 | 0.350 | 0.833 |
| 5 | 39.6 | **0.0** | 0.163 | 0.480 |

N=5'te arızalanan drone'un alanı daha çok ajana, daha eşit dağılıyor (39.6 vs
66.6 hücre); geriye kalan dengesizlik hiçbir eşikte takası haklı çıkaracak
kadar büyük olmuyor. Bu, araştırma sorusu 2 için erken bir sinyal: **ölçek
büyüdükçe yeniden-atama mekanizması daha az tetikleniyor** (atama kararlılığı
0.833 → 0.480). Bölüm 5/Faz 3'te tam OFAT verisiyle sınanacak.

## Yeniden kalibrasyon notu

Bu tarama negotiation BT alt-ağacı yazılmadan, mekanizma doğrudan çağrılarak
yapıldı. Plan Bölüm 5/Faz 0'da ikinci bir `esik_degeri` adımı var; tam BT
modeliyle orada tekrarlanacak.
