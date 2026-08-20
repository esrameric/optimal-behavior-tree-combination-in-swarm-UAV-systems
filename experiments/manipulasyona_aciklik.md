# "Manipülasyona Açıklık" Hipotez Testi

Araştırma sorusu 2: BT'nin karar mekanizması ajan sayısı arttıkça **daha kararsız/oynak mı** (manipülasyona açık) yoksa **daha kararlı mı** (dayanıklı) hale geliyor?

**Metriğin yönü:** Bölüm 6'daki *atama kararlılığı* metriği, bir drone'un atanmış alanının kaç kez değiştiğini sayar. Sayaç **yükseldikçe sistem daha oynak** olur — yani metriğin adı ölçülen şeyin tersini çağrıştırır. Aşağıdaki yorumlar bu yöne göre yapılmıştır.

## Sonuç

Ölçek N=3 → N=5 arttığında sistem **DAHA KARARLI (dayanıklı)** hale geliyor (oybirliğiyle, 16 kombinasyonun 16 tanesi).

- Ortalama değişim: **-0.177** atama değişikliği / ajan


## Kombinasyon Bazında

| kombinasyon_id         | atama_kararliligi_N3 | atama_kararliligi_N5 | atama_kararliligi_delta | bagil_degisim | sonuc        |
| ---------------------- | -------------------- | -------------------- | ----------------------- | ------------- | ------------ |
| P2a_P3c_P4b_P5abc_P6c  | 0.900                | 0.540                | -0.360                  | -0.400        | daha_kararli |
| P2c_P3c_P4b_P5c_P6c    | 0.667                | 0.400                | -0.267                  | -0.400        | daha_kararli |
| P2c_P3c_P4b_P5bc_P6c   | 0.667                | 0.400                | -0.267                  | -0.400        | daha_kararli |
| P2c_P3c_P4b_P5abc_P6a  | 0.733                | 0.480                | -0.253                  | -0.345        | daha_kararli |
| P2c_P3c_P4b_P5abc_P6b  | 0.800                | 0.560                | -0.240                  | -0.300        | daha_kararli |
| P2c_P3c_P4b_P5abc_P6c  | 0.733                | 0.540                | -0.193                  | -0.264        | daha_kararli |
| P2c_P3c_P4b_P5abcd_P6c | 0.733                | 0.540                | -0.193                  | -0.264        | daha_kararli |
| P2c_P3c_P4b_P5ac_P6c   | 0.733                | 0.540                | -0.193                  | -0.264        | daha_kararli |
| P2c_P3b_P4b_P5abc_P6c  | 0.733                | 0.560                | -0.173                  | -0.236        | daha_kararli |
| P2c_P3a_P4b_P5abc_P6c  | 0.667                | 0.500                | -0.167                  | -0.250        | daha_kararli |
| P2b_P3c_P4b_P5abc_P6c  | 0.800                | 0.640                | -0.160                  | -0.200        | daha_kararli |
| P2c_P3c_P4b_P5b_P6c    | 0.333                | 0.200                | -0.133                  | -0.400        | daha_kararli |
| P2c_P3c_P4a_P5abc_P6c  | 0.733                | 0.640                | -0.093                  | -0.127        | daha_kararli |
| P2c_P3c_P4c_P5abc_P6c  | 0.733                | 0.640                | -0.093                  | -0.127        | daha_kararli |
| P2c_P3c_P4b_P5ab_P6c   | 0.467                | 0.440                | -0.027                  | -0.057        | daha_kararli |
| P2c_P3c_P4b_P5a_P6c    | 0.467                | 0.440                | -0.027                  | -0.057        | daha_kararli |


## İkinci Ölçüm — Churn Oranı

Churn oranı, **karşılaşma başına** değişiklik olasılığını ölçer: karşılaşmaların ne kadarı gerçek bir atama değişikliğine yol açıyor? Atama kararlılığı ise **ajan başına** toplam değişikliği sayar. İkisi farklı sorular soruyor.

### ⚠ İki ölçüm ZIT yönde

- **Ajan başına** atama değişikliği: 16/16 kombinasyonda **azalıyor** → sistem daha kararlı.
- **Karşılaşma başına** değişiklik olasılığı: 10/16 kombinasyonda **artıyor** → her tekil karşılaşma daha çok değişiklik tetikliyor.

Bu bir çelişki değil, **iki farklı ölçek etkisinin** birlikte çalışmasıdır. N arttıkça (a) iş daha çok ajana bölündüğü için ajan başına düşen yeniden-atama azalıyor, ama (b) karşılaşmalar daha yoğun bir alanda ve daha dengesiz iş yükleriyle gerçekleştiği için her tekil karşılaşmanın karar üretme olasılığı artıyor.

**Sorunun cevabı ölçüm merceğine bağlı:** sürü düzeyinde bakınca ölçekle birlikte *dayanıklılık* artıyor; tekil müzakere düzeyinde bakınca *oynaklık* artıyor. Bir saldırgan tek bir karşılaşmayı manipüle etmeye çalışıyorsa N=5'te şansı daha yüksek; sürünün genel atama düzenini bozmaya çalışıyorsa daha düşük.

| kombinasyon_id         | churn_orani_N3 | churn_orani_N5 | churn_orani_delta | sonuc        |
| ---------------------- | -------------- | -------------- | ----------------- | ------------ |
| P2a_P3c_P4b_P5abc_P6c  | 0.442          | 0.156          | -0.286            | daha_kararli |
| P2b_P3c_P4b_P5abc_P6c  | 0.150          | 0.129          | -0.021            | daha_kararli |
| P2c_P3c_P4b_P5abc_P6b  | 0.033          | 0.031          | -0.002            | daha_kararli |
| P2c_P3c_P4b_P5c_P6c    | 0.000          | 0.000          | 0.000             | degismedi    |
| P2c_P3c_P4b_P5b_P6c    | 0.000          | 0.000          | 0.000             | degismedi    |
| P2c_P3c_P4b_P5bc_P6c   | 0.000          | 0.000          | 0.000             | degismedi    |
| P2c_P3a_P4b_P5abc_P6c  | 0.000          | 0.011          | 0.011             | daha_oynak   |
| P2c_P3c_P4b_P5abc_P6a  | 0.000          | 0.011          | 0.011             | daha_oynak   |
| P2c_P3c_P4b_P5ac_P6c   | 0.000          | 0.031          | 0.031             | daha_oynak   |
| P2c_P3c_P4b_P5abcd_P6c | 0.000          | 0.031          | 0.031             | daha_oynak   |
| P2c_P3c_P4b_P5a_P6c    | 0.000          | 0.031          | 0.031             | daha_oynak   |
| P2c_P3c_P4b_P5ab_P6c   | 0.000          | 0.031          | 0.031             | daha_oynak   |
| P2c_P3c_P4b_P5abc_P6c  | 0.000          | 0.031          | 0.031             | daha_oynak   |
| P2c_P3b_P4b_P5abc_P6c  | 0.000          | 0.046          | 0.046             | daha_oynak   |
| P2c_P3c_P4a_P5abc_P6c  | 0.000          | 0.050          | 0.050             | daha_oynak   |
| P2c_P3c_P4c_P5abc_P6c  | 0.000          | 0.050          | 0.050             | daha_oynak   |

