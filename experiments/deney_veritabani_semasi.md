# Ortak Deney Veritabanı Şeması — Bölüm 8

**Dosya:** [`deney_veritabani.csv`](deney_veritabani.csv)
**Araç:** `ros2 run swarm_bt_analysis experiment_db <tarama.csv> -d <veritabani.csv>`

Plan Bölüm 8, kayıt şablonunun bu çalışmanın metrikleriyle genişletilmesini ve
**iki çalışmanın tek bir ortak veritabanında** tutulmasını istiyor. Şema budur.

## Sütunlar

### Kimlik
| sütun | açıklama |
|---|---|
| `calisma` | `homojen` \| `heterojen` — **ortak veritabanının ayırt edicisi** |
| `deney_id` | Bölüm 3 şeması: `P2b_P3c_P4c_P5bc_P6c_N3` |
| `kombinasyon_id` | ölçekten arındırılmış kimlik (`..._N3` eki olmadan) |
| `tarih` | koşunun tarihi (ISO) |
| `faz` | `faz1` \| `faz2_kod` \| `faz2_gazebo` |
| **`N`** | **bu çalışmanın eklediği ölçek sütunu** |
| `P2`–`P6` | parametre uzayı (Bölüm 3) |
| `tekrar` | kaç koşunun ortalaması (Bölüm 5/Faz 1: ≥10, Faz 2: ≥5) |

### Temel metrikler (Bölüm 6)
`gorev_tamamlama_suresi`, `iletisim_yuku`, `tick_maliyeti`, `carpisma_sayisi`

### Bu çalışmaya özgü metrikler (Bölüm 8'in eklenmesini istediği)
`atama_kararliligi`, `churn_orani`, `kapsama_dengesizligi`, `karsilasma_sikligi`

### Serbest
`notlar`

## Anahtar ve tekrar koşumu

Satır anahtarı **(`calisma`, `deney_id`, `faz`)**. Bir tarama yeniden
koşulduğunda eski satır **değiştirilir**, yenisi eklenmez — veritabanı
tekrarlı koşularla şişmez (test altında).

## Heterojen çalışmanın satırları nasıl eklenir

```bash
ros2 run swarm_bt_analysis experiment_db <heterojen_tarama.csv> \
    -d experiments/deney_veritabani.csv \
    -s heterojen -n "yapilacaklar.md calismasi"
```

Şema uyuşmuyorsa araç **açıkça hata verir** (eksik sütun, bilinmeyen çalışma
etiketi, geçersiz N, tekrarlı anahtar). Sessizce bozuk satır yazmaz.

Farklı araçların kısa sütun adları (`gorev_suresi`, `tick`, `karsilasma`,
`carpisma`) otomatik eşlenir; iki çalışma aynı sütun kümesinde buluşur.

## Mevcut içerik

| calisma | faz | N | satır |
|---|---|---|---|
| homojen | faz1 | 3 | 16 |
| homojen | faz1 | 5 | 16 |
| homojen | faz2_kod | 3 | 5 |
| homojen | faz2_kod | 5 | 5 |
| homojen | faz2_gazebo | 3 | 5 |
| homojen | faz2_gazebo | 5 | 5 |

**Toplam 52 satır.** Heterojen çalışmanın satırları henüz yok (bkz.
[`faz4_heterojen_karsilastirma.md`](faz4_heterojen_karsilastirma.md)).
