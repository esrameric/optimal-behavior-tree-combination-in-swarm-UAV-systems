# Faz 2 — Gazebo Doğrulaması Özeti

**Tarih:** 2026-08-20
**Araç:** `./tools/run_phase2.sh 5 experiments/phase2_gazebo.csv`
**Ham veri:** [`phase2_gazebo.csv`](phase2_gazebo.csv) · **Analiz:** [`phase2_karsilastirma.md`](phase2_karsilastirma.md)

5 finalist × 2 ölçek × 5 tekrar, **hem Gazebo'da hem kod-seviyesinde** = **100 koşu**.
Senaryo: 120 × 120 m alan (36 hücre), `r_comm = 18 m`, arıza kapalı.
İki faz aynı config dosyasını, aynı tohumu ve **aynı kalkış konumlarını** kullanır.

## 1. Kod-seviyesi model Gazebo'yu iyi tahmin ediyor

Görev süresi bağıl sapması: **%1.0 – %16.5**, çoğunlukla %2-7.

| kombinasyon | N | süre sapması | karşılaşma sapması |
|---|---|---|---|
| Contract Net | 3 | **%1.0** | %0 |
| Contract Net | 5 | %3.2 | %0 |
| baseline | 3 | %4.5 | %0 |
| baseline | 5 | %6.2 | %0 |
| tam merkezi | 3 | %4.1 | %0 |
| tam merkezi | 5 | %4.6 | %9.7 |
| olay-güdümlü | 5 | %16.5 | %15.8 |

**Karşılaşma sayıları çoğu durumda birebir aynı** (sapma %0). Ayrık sonuçlar
(takas sayısı, churn oranı, atama kararlılığı) da örtüşüyor. Bu, Faz 1'deki
hafif kinematik modelin bu çalışma için geçerli bir soyutlama olduğunu gösterir.

## 2. Karşılaşma sıklığı ölçekle artıyor — her iki fazda da

10/10 (kombinasyon × faz) durumda `N=5 > N=3`. Artış katsayısı **2.7 – 13**.

Bu, planın Bölüm 9'da uyardığı **confound'un sayısal büyüklüğüdür**: N=5'te
ölçülen her davranış farkı, hem N'in doğrudan etkisini hem de bu yoğunluk
artışının dolaylı etkisini içerir. İki fazda da aynı büyüklükte çıkması,
artışın bir modelleme artefaktı değil geometrik bir sonuç olduğunu gösteriyor.

## 3. N-duyarlılık yönü: 31 ölçümün 23'ünde uyum (%74)

Uyuşmayan 8 ölçümün **6'sı çarpışma sayısı**. Kod-seviyesi modelde delta
her zaman 0, Gazebo'da pozitif.

**Bu beklenen ve önemli bir sonuç:** kod-seviyesi model fiziksel gövde
içermez; yakınlık ihlali yalnızca nokta-mesafe eşiğiyle sayılır. Gazebo'da
gerçek gövdeler var, ölçekle birlikte yakın geçişler artıyor. **Çarpışma
metriği kod-seviyesinde güvenilir değil; Gazebo doğrulaması bunun için şart.**

Kalan 2 uyuşmazlık `P2c_P3c_P4b_P5abc_P6a` (periyodik yoklama) kombinasyonunda
takas/churn/atama kararlılığı: kod-seviyesinde N=5'te hiç takas olmuyor,
Gazebo'da 0.2 takas/koşu oluyor. Sebep, Gazebo'daki zamanlama farklarının
yoklama anlarını kaydırması — periyodik yoklamanın kayıplı doğası (V-P6a)
fiziksel zamanlamaya duyarlı.

## 4. Tam merkezi mimari Gazebo'da N=5'te tamamlanamıyor

| kombinasyon | N | Gazebo tamamlanma | kod-seviyesi tamamlanma |
|---|---|---|---|
| `P2a_P3c_P4b_P5abc_P6c` | 5 | **0.80** | 1.00 |
| diğer 9 (kombinasyon, ölçek) | — | 1.00 | 1.00 |

5 Gazebo koşusundan biri kapsamayı tamamlayamadı — yalnızca merkezi mimaride
ve yalnızca N=5'te. Faz 1'in "merkezi koordinasyon ölçekle bozuluyor"
bulgusunun (görev süresi N=3'te −3.86 s, N=5'te +12.74 s) fiziksel karşılığı.
