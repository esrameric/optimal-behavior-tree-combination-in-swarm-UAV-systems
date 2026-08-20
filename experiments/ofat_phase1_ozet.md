# OFAT Taraması — Faz 1 Özeti

**Tarih:** 2026-08-20
**Araç:** `ros2 run swarm_bt_sim ofat_sweep --repetitions 10`
**Ham veri:** [`ofat_phase1.csv`](ofat_phase1.csv) · **Analiz:** [`ofat_phase1_rapor.md`](ofat_phase1_rapor.md)

16 kombinasyon × 2 ölçek × 10 tekrar = **320 koşu**, ~33 saniye.
Baseline: `P2c_P3c_P4b_P5abc_P6c`. Arıza enjeksiyonu açık (görev ortası, rastgele drone).

## Manşet bulgu — P3 ölçeğe en duyarlı eksen

**Contract Net (P3b), N=3'te iyi, N=5'te kötü:**

| metrik | N=3 (baseline'a göre) | N=5 (baseline'a göre) |
|---|---|---|
| görev tamamlama süresi | **−18.20 s** (iyileşme) | **+13.53 s** (kötüleşme) |
| tick maliyeti | −182 | +135 |
| çarpışma sayısı | −0.10 | +0.20 |

Aynı parametre değişimi iki ölçekte **zıt yönde** etki ediyor — planın Bölüm 4'te
tam olarak aradığı bulgu. Yorum: Contract Net'in sıralı ihalesi (sütunlar soldan
sağa açılır) az sayıda ajanla iyi çalışıyor; ajan sayısı arttıkça duyuru
sırasının yarattığı yanlılık büyüyor ve CBBA'nın en-iyi-önce yaklaşımının
gerisinde kalıyor.

**Statik bölme (P3a)** de churn oranı ve çarpışma sayısında yön çatışması
gösteriyor.

## Eksen bazında ölçek duyarlılığı sıralaması

| eksen | ölçülen | yön çatışması | ölçek eşiği | hiç etkisiz |
|---|---|---|---|---|
| **P3** (alan atama) | 16 | **5** | 3 | 2 |
| **P6** (tetikleme) | 16 | 1 | 2 | 9 |
| **P2** (koordinasyon) | 16 | 0 | 1 | 9 |
| **P5** (iletişim) | 56 | 0 | 3 | 35 |
| **P4** (BT mimarisi) | 16 | 0 | 0 | **16** |

## Bilinen sınırlılık — P4 bu taramada etkisiz

P4'ün 16/16 ölçümde etkisiz çıkması bir bulgu **değil**, bilinen bir eksiklik:
bu aşamada BT ağacı simülasyonu henüz **sürmüyor**. Karşılaşma anındaki karar
doğrudan mekanizma çağrılarıyla yürütülüyor (`AreaSwapNegotiator`), dolayısıyla
üç BT XML varyantı arasındaki yapısal fark davranışa yansımıyor.

Negotiation alt-ağacı ve BT tick döngüsü plan Bölüm 5/Faz 0'da yazılacak;
tarama Faz 1'de o modelle **yeniden koşulacak** ve P4 satırları o zaman anlam
kazanacak. Bu tarama, diğer dört eksen için geçerli bir ilk kesittir.

## Diğer gözlemler

- **P2b ≡ P2c**: hiyerarşik hibrit, bu ölçeklerde tam dağıtıktan ayırt
  edilemiyor (bkz. README V16). Tabloda P2'nin 9/16 "hiç etkisiz" satırı bunun
  sonucu.
- **P5 büyük ölçüde etkisiz** (35/56): stigmerjinin görev süresine etkisi kesin
  partisyonda bağlamıyor (README V15); ortak tarama dalı gelince yeniden
  ölçülecek.
- **P6a (periyodik yoklama)** N=5'te iletişim yükünü belirgin artırıyor
  (+48.4), N=3'te azaltıyor (−1.8) — yoklama, yoğunluk arttıkça pahalılaşıyor.
