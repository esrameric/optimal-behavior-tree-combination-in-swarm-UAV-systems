# Confound Kontrol Deneyi — Bölüm 9

**Plan Bölüm 9:** *"N=5'te daha fazla karşılaşma olması hem N'in doğrudan etkisi
hem de yoğunluk artışının dolaylı etkisi — raporda bu ikisini ayırt etmeye
çalış (karşılaşma sıklığını sabitleyip N'i değiştiren bir kontrol deneyi zor
ama düşünmeye değer)."*

**Kontrol deneyi koşuldu.** Ham veri: [`confound_kontrol.csv`](confound_kontrol.csv)
Araç: `ros2 run swarm_bt_sim confound_control --repetitions 10`

## Yöntem

`r_comm` karşılaşma sıklığını doğrudan belirliyor (Bölüm 1 kalibrasyonu bunu
ölçtü). Öyleyse her ölçek için `r_comm`'u karşılaşma sıklığı **aynı** olacak
şekilde ayarlarsak, kalan fark yalnızca N'in doğrudan etkisidir.

| kol | N=3 | N=5 | karşılaşma |
|---|---|---|---|
| **serbest** | r_comm = 60 m | r_comm = 60 m | 2.4 → 7.3 (**3.0 kat artıyor**) |
| **eşitli** | r_comm = 60 m | r_comm = **20 m** | 2.4 → 2.3 (**sabit**) |

Eşitli kolun `r_comm` değerleri aramayla değil, Bölüm 1'de zaten ölçülmüş
kalibrasyon tablosundan okundu ([`calibration_rcomm_bt.csv`](calibration_rcomm_bt.csv)):
N=5 için 20 m, N=3'ün 2.4 karşılaşmasına en yakın değeri (2.3) veriyor.

## Ayrıştırma

`N=3 → N=5` değişimi üç sütunda:

| metrik | toplam etki (serbest) | **saf N etkisi** (eşitli) | yoğunluğun payı |
|---|---|---|---|
| görev tamamlama süresi | -172.970 | -192.780 | +19.810 |
| atama kararlılığı (değişiklik/ajan) | -0.193 | -0.313 | +0.120 |
| iletişim yükü | +183.100 | -79.600 | +262.700 |
| churn oranı | +0.031 | +0.000 | +0.031 |
| kabul edilen takas | +0.200 | +0.000 | +0.200 |

## Okuma

### 1. Görev süresi — yoğunluk, paralelliğin kazancını törpülüyor
Saf N etkisi **−192.8 s**; ölçülen toplam etki ise **−173.0 s**. Aradaki
**+19.8 s**, artan karşılaşma sıklığının getirdiği koordinasyon yüküdür.
Yani N=5'in hız kazancının yaklaşık **%10'u** yoğunluk artışına gidiyor.

### 2. Atama kararlılığı — bulgu confound'un ARTEFAKTI DEĞİL, tersine
Faz 3'ün ana bulgusu, ajan başına atama değişikliğinin ölçekle azalmasıydı
(daha kararlı). Kontrol deneyi bunu **güçlendiriyor**:

- ölçülen azalma: **−0.193**
- saf N etkisi: **−0.313** (daha büyük azalma)
- yoğunluğun payı: **+0.120** (ters yönde)

Yoğunluk artışı kararlılığı *azaltıyor*, ama N'in doğrudan etkisi bunu fazlasıyla
telafi ediyor. **Sistem ölçekle daha kararlı hale geliyor, ve confound
giderilince bu daha da belirgin.**

### 3. İletişim yükü — artış TAMAMEN confound
| | N=3 | N=5 | değişim |
|---|---|---|---|
| serbest | 142.8 | 325.9 | **+183.1** |
| eşitli | 142.8 | 63.2 | **−79.6** |

Karşılaşma sıklığı sabitlendiğinde daha kalabalık sürü **daha az** mesaj
üretiyor. Ölçülen +183 mesajlık artışın tamamı yoğunluk artışından geliyor,
ajan sayısından değil. Bu, "büyük sürü daha çok konuşur" sezgisinin bu
senaryoda **yanlış** olduğunu gösteriyor.

### 4. Takas mekanizması bir YOĞUNLUK etkisi
Eşitli kolda N=5'te takas sayısı ve churn oranı **sıfır** — serbest kolda
0.2 takas / 0.031 churn vardı. Yani "takas mekanizması ölçekle devreye giriyor"
bulgusu (Bölüm 5/Faz 1) aslında **karşılaşma sıklığının artmasıyla** devreye
giriyor; ajan sayısının kendisi takası tetiklemiyor.

## Sonuç

Planın "ayırt etmeye çalış" dediği iki etki **ayrıştırıldı** ve dördü de farklı
davranıyor:

| bulgu | asıl kaynağı |
|---|---|
| görev süresi kısalıyor | **N'in kendisi** (yoğunluk %10 törpülüyor) |
| atama kararlılığı artıyor | **N'in kendisi** (yoğunluk ters yönde çalışıyor) |
| iletişim yükü artıyor | **tamamen yoğunluk** |
| takas mekanizması devreye giriyor | **tamamen yoğunluk** |

Rapor yazılırken bu ayrım korunmalı: ilk iki bulgu ölçek hakkında, son ikisi
karşılaşma sıklığı hakkındadır.
