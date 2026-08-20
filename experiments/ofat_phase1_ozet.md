# OFAT Taraması — Faz 1 Özeti (tam BT modeliyle)

**Tarih:** 2026-08-20
**Araç:** `ros2 run swarm_bt_sim ofat_sweep --repetitions 10 --phase faz1`
**Ham veri:** [`ofat_phase1.csv`](ofat_phase1.csv) · **Analiz:** [`ofat_phase1_rapor.md`](ofat_phase1_rapor.md)

16 kombinasyon × 2 ölçek × **10 tekrar** = **320 koşu**, ~5.5 dakika.
Baseline: `P2c_P3c_P4b_P5abc_P6c`. Arıza enjeksiyonu açık (görev ortası, rastgele drone).
Kalibre değerler: `r_comm = 60 m`, `esik_degeri = 0.10`.

## Manşet bulgular

### 1. Merkezi koordinasyon (P2a) ölçekle bozuluyor

| metrik | N=3 (baseline'a göre) | N=5 (baseline'a göre) |
|---|---|---|
| görev tamamlama süresi | **−3.86 s** (iyileşme) | **+12.74 s** (kötüleşme) |
| tick maliyeti | −38.6 | +127.4 |

Bu, araştırma sorusu 2'ye doğrudan cevap veren bulgudur: **tam merkezi mimari
N=3'te dağıtıktan iyi, N=5'te kötü.** Merkez, her koordinasyon adımında tüm
ajanlardan rapor topluyor; ajan sayısı arttıkça bu yük ve tek karar noktasının
darboğazı baskın hale geliyor. Planın Bölüm 5/Faz 3'te sorduğu *"tam merkezi
mimari N arttıkça mı bozuluyor, tam dağıtık mı daha dayanıklı çıkıyor?"*
sorusunun cevabı: **evet, merkezi bozuluyor.**

### 2. Contract Net (P3b) ölçekle yön değiştiriyor

| metrik | N=3 | N=5 |
|---|---|---|
| görev tamamlama süresi | **−17.45 s** | **+6.48 s** |
| iletişim yükü | −3.2 | **+98.4** |
| tick maliyeti | −174.5 | +64.8 |

Sıralı ihale (sütunlar soldan sağa açılır) az ajanla iyi; ajan sayısı arttıkça
duyuru sırasının yarattığı yanlılık büyüyor ve CBBA'nın en-iyi-önce yaklaşımının
gerisinde kalıyor. İletişim yükündeki 30 kat fark özellikle çarpıcı.

Bu bulgu **BT karar katmanı öncesindeki taramada da aynı yönde çıkmıştı**
(o zaman N=3: −18.2 s, N=5: +13.5 s) — iki bağımsız model kesitinde tekrarlandı.

### 3. Takas mekanizması ölçekle devreye giriyor

N-duyarlılık tablosunda `churn_orani` sütunu çoğu kombinasyonda `-` (tanımsız):
**N=3'te değer sıfır.** Takas hiç tetiklenmiyor; yeniden-atama tamamen arıza
devralmasından geliyor. N=5'te churn oranı sıfırdan farklı.

Yani "manipülasyona açıklık" sorusunun cevabı bu ölçek aralığında şudur:
**N=3'te dinamik yeniden-atama pratikte yok; N=5'te var.** Atama kararlılığı da
buna paralel: N=3→N=5 arası ortalama %26 değişiyor (baseline).

## Eksen sıralaması

| eksen | ölçülen | yön çatışması | ölçek eşiği | hiç etkisiz |
|---|---|---|---|---|
| **P3** (alan atama) | 16 | **4** | 3 | 2 |
| **P2** (koordinasyon) | 16 | 2 | 4 | 3 |
| **P4** (BT mimarisi) | 16 | 2 | 6 | 2 |
| **P5** (iletişim) | 56 | 2 | 6 | 22 |
| **P6** (tetikleme) | 16 | 1 | 4 | 4 |

**P4 artık ölçülebilir.** Bir önceki taramada BT ağacı simülasyonu sürmediği
için P4 16/16 ölçümde etkisizdi; şimdi 2/16. P4a ve P4c, N=5'te atama
kararlılığını ve çarpışma sayısını belirgin değiştiriyor.

## En ölçek-duyarlı kombinasyonlar

| kombinasyon | ortalama N-duyarlılık |
|---|---|
| `P2c_P3b_P4b_P5abc_P6c` (Contract Net) | **1.853** |
| `P2c_P3c_P4c_P5abc_P6c` (olay-güdümlü BT) | 1.078 |
| `P2c_P3c_P4b_P5abc_P6a` (periyodik yoklama) | 1.058 |
| `P2c_P3a_P4b_P5abc_P6c` (statik bölme) | 1.051 |
| `P2c_P3c_P4b_P5abc_P6c` (**baseline**) | 0.971 |
