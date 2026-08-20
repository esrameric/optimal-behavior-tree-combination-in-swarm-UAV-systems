# `r_comm` Kalibrasyonu — Bölüm 1

**Tarih:** 2026-08-20
**Araç:** `ros2 run swarm_bt_sim calibrate_rcomm --repetitions 10`
**Ham veri:** [`calibration_rcomm.csv`](calibration_rcomm.csv)

## Karar

| Parametre | Değer | Gerekçe |
|---|---|---|
| **`r_comm`** | **60 m** | 400 m'lik alan kenarının **%15'i** — planın önerdiği %10-20 bandının ortası |
| Mission alanı | 400 × 400 m, **N'den bağımsız sabit** | Bölüm 1 gereği |
| Hücre boyutu | 20 m (20×20 = 400 tarama hücresi) | |

Ölçülen değerler (10 tekrar, hız sapması %5):

| N | karşılaşma (ort ± std) | görev süresi (ort) |
|---|---|---|
| 3 | 2.6 ± 0.5 | 290.6 s |
| 5 | 5.3 ± 0.6 | 166.8 s |

`r_comm = 60 m`, karşılaşma sayısının menzile **gerçekten tepki verdiği** dar
bandın (50-70 m) içindedir. Bu bandın altında (≤40 m) sayı geometrik tabana
(N-1 sınır çifti) sabitlenir ve varyans sıfırdır — menzil hiçbir şey yapmıyor
demektir. Üstünde (≥80 m) çiftler menzilde daha uzun kaldığı için ayrı *giriş*
olayları birleşir ve sayı tekrar tabana düşer.

## Kalibrasyon sırasında çıkan iki model hatası

Tarama, iki gerçek modelleme hatasını açığa çıkardı. İkisi de düzeltilmeden
kalibrasyon anlamsız olurdu.

### 1. Rijit formasyon → `r_comm`'un hiçbir etkisi yok

İlk ölçümde karşılaşma sayısı `r_comm` 20 m'den 140 m'ye çıkarılmasına rağmen
**hiç değişmedi** (N=3 için hep 2, N=5 için hep 4). Sebep: tüm ajanlar tam
olarak aynı hızda, aynı fazda, eşit boyutlu bölgelerde uçuyordu; şeritler
boyunca sabit sütun farkıyla kilitli bir formasyonda ilerliyorlardı.

Düzeltme: ajan başına tohumlanmış hız sapması (`speed_jitter`, varsayılan
±%5). Gerçek dronelar da tam olarak aynı hızda uçmaz. Sapma açıldığında
karşılaşma sayısı 2 → 21 seviyesine çıktı. Tohumlanmış olması, planın Bölüm
5/Faz 1'de istediği ≥10 tekrarın tekrarlanabilir olmasını da sağlar.

### 2. Eşiğe teğet geçiş → sahte karşılaşma yağmuru (chattering)

Sapma eklendikten sonra eğri `r_comm` ile **monoton olmayan**, 40 m periyotlu
ani sıçramalar gösterdi (20/60/100/140/180 m'de tepe). İzleme çıktısı sebebi
gösterdi: biçerdöver deseninde bitişik sütunlarda paralel süpüren iki drone
arasındaki mesafe **tam olarak** hücre boyutunun katıdır, ve hız sapması
yüzünden eşiğin iki yanında salınır. Ölçülen örnek — `r_comm = 20 m`:

```
t=    0.1  KARSILASMA (1,2) d=20.00
t=    2.1  KARSILASMA (1,2) d=20.00
t=    4.1  KARSILASMA (1,2) d=20.00        ... her 2 saniyede bir, 130 kez
```

Tek bir çift, tek bir yakınlaşma boyunca 130 kez "karşılaştı". Bu, Bölüm 6'daki
*karşılaşma sıklığı* ve *churn oranı* metriklerini tamamen bozardı.

Düzeltme: `EncounterDetector`'a histerezis (giriş eşiği `r_comm`, çıkış eşiği
`r_comm × 1.1`). Fiziksel olarak da doğrusu budur — gerçek bir telsiz
bağlantısı da, iki drone arasındaki müzakere de her iki saniyede bir yeniden
kurulmaz. Düzeltmeden sonra eğri düzgünleşti.

## Sınırlılık ve yeniden kalibrasyon notu

Bu kalibrasyon, **negotiation alt-ağacı henüz yokken** yapıldı: ajanlar yalnızca
kendi şeritlerini tarıyor, alan takası yapmıyor. Bu yüzden karşılaşmalar
neredeyse tamamen komşu şerit sınırlarından doğuyor ve koşu başına 2-5 ile
sınırlı. Alan takası ve arıza devralma devreye girdiğinde ajanlar alan boyunca
hareket edecek ve karşılaşma sıklığının artması bekleniyor.

Plan Bölüm 5/Faz 0 zaten ikinci bir `r_comm` kalibrasyon adımı içeriyor;
bu tarama tam negotiation modeliyle orada tekrarlanacak.
