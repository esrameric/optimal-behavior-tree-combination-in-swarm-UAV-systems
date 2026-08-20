# Faz 4 — Heterojen Çalışmayla Karşılaştırma

**Durum:** karşılaştırma **yapılamadı** — heterojen çalışmanın (`yapilacaklar.md`)
verisi bu repoda yok. Bu belge (a) karşılaştırmanın nasıl yapılacağını sabitler,
(b) bu çalışmanın karşılaştırılabilir bulgularını listeler, (c) sınanacak
hipotezleri yazar.

## Karşılaştırma nasıl yapılacak

Heterojen çalışmanın OFAT tarama CSV'si elde olduğunda tek komut yeterli:

```bash
ros2 run swarm_bt_analysis cross_study \
    experiments/ofat_phase1.csv <heterojen_tarama.csv> \
    -o experiments/faz4_sonuc.md
```

Araç, iki taramanın **ortak kombinasyonlarını** eşleştirir ve her metrik için
N=3→N=5 değişiminin iki çalışmada aynı yönde olup olmadığını raporlar. Zıt yön,
**rol heterojenliğinin o parametrenin ölçek davranışını değiştirdiği** anlamına
gelir — Faz 4'ün aradığı bulgu tam olarak budur.

### Gereken veri şeması
Heterojen tarama CSV'si şu sütunları taşımalı (bu çalışmanın `ofat_sweep`
çıktısıyla aynı): `kombinasyon_id`, `N`, `tekrar`, `P2`–`P6` ve Bölüm 6
metrikleri. Parametre uzayı farklıysa yalnızca ortak kombinasyonlar
karşılaştırılır; hiç ortak yoksa araç bunu açıkça söyler.

## Bu çalışmanın karşılaştırılabilir bulguları

| # | Bulgu | Ölçülen |
|---|---|---|
| 1 | Merkezi koordinasyon (P2a) ölçekle bozuluyor | N=3: −3.86 s, N=5: +12.74 s (baseline'a göre) |
| 2 | Contract Net (P3b) ölçekle yön değiştiriyor | N=3: −17.45 s, N=5: +6.48 s |
| 3 | En duyarlı eksen P3, en az duyarlı P2 | 1.452 vs 0.896 |
| 4 | Ajan başına atama değişikliği ölçekle **azalıyor** | 16/16 kombinasyon, ortalama −0.177 |
| 5 | Karşılaşma başına değişiklik olasılığı ölçekle **artıyor** | 10/16 kombinasyon |
| 6 | Takas mekanizması N=3'te hiç tetiklenmiyor | churn oranı N=3'te tüm kombinasyonlarda 0 |
| 7 | Karşılaşma sıklığı N ile 2.7–13 kat artıyor | her iki fazda da doğrulandı |

## Sınanacak hipotezler

**H1 — Merkezi bozulma heterojen sürüde daha erken başlar.**
Rol heterojenliği merkezin karar yükünü artırır (farklı rol tipleri için farklı
tahsis kuralları). Bulgu 1'in N=5'teki kötüleşmesinin heterojen sürüde daha
büyük çıkması beklenir.

**H2 — Atama kararlılığındaki ölçek etkisi (bulgu 4) tersine dönebilir.**
Homojen sürüde iş her ajana eşit bölünebildiği için ajan başına yeniden-atama
ölçekle azalıyor. Rol heterojenliğinde bir görev yalnızca belirli rollere
verilebildiğinden, ajan sayısı artsa bile uygun ajan sayısı artmayabilir; bu
durumda ajan başına atama değişikliği **azalmaz, artabilir**.

**H3 — P3'ün baskınlığı (bulgu 3) heterojen sürüde daha da güçlenir.**
Alan atama, rol kısıtları eklendiğinde daha zor bir problem olur.

**H4 — Karşılaşma sıklığı artışı (bulgu 7) rol dağılımından bağımsızdır.**
Artış geometrik bir sonuç (sabit alanda daha çok ajan); rol heterojenliği bunu
değiştirmemeli. Bu, karşılaştırmanın **kontrol ölçümüdür**: H4 tutmuyorsa iki
çalışmanın senaryoları karşılaştırılabilir değil demektir.

## Ortak veritabanı

Plan Bölüm 8, iki çalışmanın tek bir deney veritabanında tutulmasını istiyor.
Bu çalışmanın şeması `experiments/deney_veritabani_semasi.md` içinde; heterojen
çalışmanın satırları `calisma` sütunuyla ayırt edilerek aynı tabloya eklenebilir.
