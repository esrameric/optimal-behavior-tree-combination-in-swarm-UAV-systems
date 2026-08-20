# Sonuçlar — Homojen Keşif Sürüsü, Ölçek-Duyarlılık Araştırması

Bu belge planın iki araştırma sorusunu, toplanan veriye dayanarak cevaplar.
Her iddianın yanında onu üreten ölçüm dosyası vardır.

**Kapsam:** 320 koşu (Faz 1) + 100 koşu (Faz 2, Gazebo dahil) + 80 koşu
(Bölüm 9 kontrolleri) + kalibrasyon taramaları. 615 otomatik test.

---

## Araştırma Sorusu 1 — Optimal BT + koordinasyon kombinasyonu nedir?

**Cevap: `P2b_P3c_P4b_P5abc_P6c`** — hiyerarşik hibrit koordinasyon + CBBA
alan atama + özdeş dağıtık BT + tam iletişim + saf olay-tetiklemeli.

Görev tamamlama süresi (arızalı senaryo, 10 tekrar):

| kombinasyon | N=3 | N=5 |
|---|---|---|
| **P2b** hiyerarşik hibrit | **458.34 s** | **290.52 s** |
| P2c tam dağıtık (baseline) | 468.73 s | 295.76 s |
| P2a tam merkezi | 464.87 s | 308.50 s |

Kaynak: [`ofat_phase1.csv`](experiments/ofat_phase1.csv),
[`parametre_duyarliligi.md`](experiments/parametre_duyarliligi.md)

### Bileşen bazında gerekçe

| eksen | seçim | neden |
|---|---|---|
| P2 | **b** hiyerarşik hibrit | Her iki ölçekte de en hızlı. Lider kümeyi bütün olarak planlıyor; dağıtıkta aynı iş yalnızca ikili müzakereyle, karşılaşma sırasına bağlı yapılıyor. Ama üstünlüğü ölçekle azalıyor (−10.4 s → −5.2 s). |
| P3 | **c** CBBA | En-iyi-önce tahsis, duyuru sırasından bağımsız. Contract Net (P3b) N=3'te daha iyi ama **N=5'te kötüleşiyor** — en ölçek-duyarlı seçim. |
| P4 | **b** özdeş dağıtık BT | P4a (merkezi) ve P4c (olay-güdümlü) N=5'te atama kararlılığını ve çarpışma sayısını artırıyor, karşılığında kazanç vermiyor. |
| P5 | **abc** tam iletişim | **Intent yayını (c) zorunlu**: onsuz N=3 koşularının yalnızca %20'si kapsamayı tamamlıyor. Kulak misafiri (d) ölçülebilir katkı vermiyor. |
| P6 | **c** saf olay-tetiklemeli | P6b (her tick) 55 kat daha fazla koordinasyon kararı üretiyor, karşılığında kazanç yok. P6a (periyodik) N=5'te iletişim yükünü artırıyor. |

### Kritik bağımlılık — intent yayını

| kombinasyon | N | görev süresi | kapsama tamamlanma |
|---|---|---|---|
| `P5abc` | 3 | 468.7 s | **1.00** |
| `P5a` / `P5ab` (intent yok) | 3 | 2495.8 s | **0.20** |
| `P5abc` | 5 | 295.8 s | 1.00 |
| `P5a` / `P5ab` | 5 | 1657.3 s | 0.50 |

Intent yayını olmadan boşta kalan ajan sahipsiz alanın varlığını öğrenemiyor;
devralma yalnızca karşılaşma anında mümkün oluyor ve karşılaşma gerçekleşmezse
arızalanan drone'un alanı **hiç taranmıyor**.

---

## Araştırma Sorusu 2 — Aynı kombinasyon ölçekle nasıl davranıyor?

### Cevap ölçüm merceğine bağlı — ve bu, bulgunun kendisi

| mercek | sonuç | veri |
|---|---|---|
| **ajan başına** atama değişikliği | ölçekle **azalıyor** → sürü daha **kararlı** | 16/16 kombinasyon, ortalama −0.177 |
| **karşılaşma başına** değişiklik olasılığı | ölçekle **artıyor** → tekil müzakere daha **oynak** | 10/16 kombinasyon |

Kaynak: [`manipulasyona_aciklik.md`](experiments/manipulasyona_aciklik.md)

Bu bir çelişki değil, iki ölçek etkisinin birlikte çalışmasıdır: N arttıkça
(a) iş daha çok ajana bölündüğü için ajan başına yeniden-atama azalıyor,
(b) karşılaşmalar daha yoğun bir alanda gerçekleştiği için her tekil
karşılaşmanın karar üretme olasılığı artıyor.

**Pratik okuma:** bir saldırgan **tek bir karşılaşmayı** manipüle etmeye
çalışıyorsa N=5'te şansı daha yüksek; sürünün **genel atama düzenini** bozmaya
çalışıyorsa daha düşük.

### Bulgu confound'un artefaktı değil — kontrol deneyi bunu güçlendiriyor

Karşılaşma sıklığı sabitlenerek N'in saf etkisi ölçüldü
([`confound_kontrol.md`](experiments/confound_kontrol.md)):

| metrik | toplam etki | **saf N etkisi** | yoğunluğun payı |
|---|---|---|---|
| görev süresi | −172.97 s | −192.78 s | +19.81 s |
| atama kararlılığı | −0.193 | **−0.313** | +0.120 |
| iletişim yükü | +183.10 | **−79.60** | +262.70 |
| kabul edilen takas | +0.200 | **0.000** | +0.200 |

- Kararlılık artışı **N'in kendisinden**; yoğunluk ters yönde çalışıyor.
- İletişim yükü artışı **tamamen yoğunluktan**; sıklık sabitken daha kalabalık
  sürü **daha az** mesaj üretiyor.
- Takas mekanizmasının devreye girmesi **tamamen yoğunluktan**; ajan sayısının
  kendisi takası tetiklemiyor.

### Trend N=10'a kadar sürüyor, doyuma giderek

| N | atama kararlılığı | görev süresi |
|---|---|---|
| 3 | 0.733 | 468.73 s |
| 5 | 0.540 | 295.76 s |
| 7 | 0.443 | 246.78 s |
| 10 | 0.410 | 197.69 s |

Kaynak: [`olcek_genisletme.md`](experiments/olcek_genisletme.md)

### Ölçeklenebilirlik sınırı: N ≈ 5

Ajan başına iş yükü sabit tutulduğunda (orantılı alan) drone eklemek görevi
**N=5'ten sonra yavaşlatıyor**: 453 s (N=5) → 656 s (N=7). İletişim yükü N=3'ten
N=10'a **30 kat** artıyor. Bu koordinasyon mimarisi N≈5'e kadar ölçekleniyor.

**Sürü ölçekle daha kararlı ama daha verimsiz hale geliyor.**

---

## Parametrelerin ölçek duyarlılığı

| eksen | ortalama N-duyarlılık | yorum |
|---|---|---|
| **P3** alan atama | **1.452** | en duyarlı; Contract Net yön değiştiriyor |
| P4 BT mimarisi | 1.013 | |
| P6 tetikleme | 0.949 | |
| P5 iletişim | 0.935 | |
| **P2** koordinasyon | **0.896** | en az duyarlı — ama merkezi mimari yön değiştiriyor |

**Planın açık sorusu — merkezi mi dağıtık mı?** Tam merkezi mimari N=3'te
dağıtıktan **iyi** (−3.86 s), N=5'te **kötü** (+12.74 s). **Merkezi koordinasyon
ölçekle bozuluyor; tam dağıtık daha dayanıklı.** Gazebo'da da doğrulandı:
merkezi mimari N=5'te 5 koşudan yalnızca 4'ünü tamamlayabildi.

---

## Kod-seviyesi model geçerli mi?

Evet. Faz 2'de aynı senaryo hem kod-seviyesinde hem Gazebo'da koşuldu:

- görev süresi bağıl sapması **%1–16.5**, çoğunlukla %2–7
- karşılaşma sayıları çoğu durumda **birebir aynı**
- N-duyarlılık yönü 31 ölçümün 23'ünde (%74) uyuşuyor

Uyuşmayan 8 ölçümün 6'sı **çarpışma sayısı**: kod-seviyesi model fiziksel gövde
içermez. **Çarpışma metriği kod-seviyesinde güvenilir değil.**

Kaynak: [`phase2_ozet.md`](experiments/phase2_ozet.md)

---

## Bilinen sınırlılıklar

1. **PX4 SITL uçuş yığını kullanılmadı** (README V18). Gazebo fiziği ve hız
   kontrollü gövdeler var, uçuş dinamiği yok. `IPositionSource` arkasında
   kapsanmış bir değişiklik.
2. **Faz 2 senaryosu küçültüldü** (400 → 120 m, README V20) çünkü Gazebo gerçek
   zamanlı koşuyor. Kod-seviyesi karşılaştırma koşuları da aynı dosyayla
   yapıldı, karşılaştırma geçerli.
3. **Heterojen çalışmayla karşılaştırma yapılamadı** — o çalışmanın verisi bu
   repoda yok. Karşılaştırma aracı ve hipotezler hazır
   ([`faz4_heterojen_karsilastirma.md`](experiments/faz4_heterojen_karsilastirma.md)).
4. **Takas mekanizması N=3'te ölü**: hiçbir eşik değerinde takas kabul
   edilmiyor. Yeniden-atama N=3'te tamamen arıza devralmasından geliyor.
5. **Stigmerjinin (P5b) görev süresine etkisi ölçülemedi** (README V15):
   bölgeler kesin partisyon olduğu için mükerrer tarama oluşmuyor. Ortak tarama
   dalı eklendi ama tetiklenme sıklığı düşük.
