# OFAT Taramasi — Olcek Duyarliligi

- Baseline kombinasyon: `P2c_P3c_P4b_P5abc_P6c`
- Olcek degerleri: N=3, N=5
- Kombinasyon sayisi: 16
- Toplam satir: 32
- Kombinasyon x olcek basina tekrar: 10

## 1. Olcege Duyarli Parametreler (yon CATISMASI)

Plan Bolum 4: bir parametre degisimi iki N degerinde ZIT yonde etki ediyorsa, o parametre olcege duyarlidir — bulgunun kendisi budur.

Burada yalnizca IKI TARAFTA DA etki olan ve yonleri zit olan olcumler listelenir. "Bir olcekte etki var, digerinde yok" durumu ayri baslikta verilir: o bir catisma degil, etkinin ortaya ciktigi olcek esigidir.

| kombinasyon_id        | degisen_eksen | metrik                 | delta_N3 | delta_N5 |
| --------------------- | ------------- | ---------------------- | -------- | -------- |
| P2a_P3c_P4b_P5abc_P6c | P2            | gorev_tamamlama_suresi | -3.860   | 12.740   |
| P2a_P3c_P4b_P5abc_P6c | P2            | tick_maliyeti          | -38.600  | 127.400  |
| P2c_P3b_P4b_P5abc_P6c | P3            | gorev_tamamlama_suresi | -17.450  | 6.480    |
| P2c_P3b_P4b_P5abc_P6c | P3            | iletisim_yuku          | -3.200   | 98.400   |
| P2c_P3b_P4b_P5abc_P6c | P3            | tick_maliyeti          | -174.500 | 64.800   |
| P2c_P3b_P4b_P5abc_P6c | P3            | carpisma_sayisi        | -0.100   | 0.200    |
| P2c_P3c_P4a_P5abc_P6c | P4            | karsilasma_sikligi     | -0.100   | 0.200    |
| P2c_P3c_P4b_P5abc_P6a | P6            | iletisim_yuku          | -1.700   | 23.200   |
| P2c_P3c_P4b_P5bc_P6c  | P5            | carpisma_sayisi        | 0.100    | -0.100   |
| P2c_P3c_P4b_P5c_P6c   | P5            | carpisma_sayisi        | 0.100    | -0.100   |
| P2c_P3c_P4c_P5abc_P6c | P4            | karsilasma_sikligi     | -0.100   | 0.200    |


## 2. Eksen Bazinda Ozet

| degisen_eksen | olculen | yon_catismasi | tek_tarafli | etkisiz |
| ------------- | ------- | ------------- | ----------- | ------- |
| P3            | 16      | 4             | 3           | 2       |
| P2            | 16      | 2             | 4           | 3       |
| P4            | 16      | 2             | 6           | 2       |
| P5            | 56      | 2             | 6           | 22      |
| P6            | 16      | 1             | 4           | 4       |


### 2b. Olcek Esigi Gosteren Olcumler

Etki yalnizca bir olcekte ortaya cikiyor; parametre o esigin altinda/ustunde davranis degistiriyor.

| kombinasyon_id        | degisen_eksen | metrik             | delta_N3 | delta_N5 |
| --------------------- | ------------- | ------------------ | -------- | -------- |
| P2a_P3c_P4b_P5abc_P6c | P2            | atama_kararliligi  | 0.167    | 0.000    |
| P2a_P3c_P4b_P5abc_P6c | P2            | karsilasma_sikligi | 0.000    | 0.400    |
| P2a_P3c_P4b_P5abc_P6c | P2            | carpisma_sayisi    | 0.000    | 0.100    |
| P2b_P3c_P4b_P5abc_P6c | P2            | karsilasma_sikligi | -0.200   | 0.000    |
| P2c_P3a_P4b_P5abc_P6c | P3            | churn_orani        | 0.000    | -0.020   |
| P2c_P3b_P4b_P5abc_P6c | P3            | atama_kararliligi  | 0.000    | 0.020    |
| P2c_P3b_P4b_P5abc_P6c | P3            | churn_orani        | 0.000    | 0.015    |
| P2c_P3c_P4a_P5abc_P6c | P4            | atama_kararliligi  | 0.000    | 0.100    |
| P2c_P3c_P4a_P5abc_P6c | P4            | churn_orani        | 0.000    | 0.019    |
| P2c_P3c_P4a_P5abc_P6c | P4            | carpisma_sayisi    | 0.000    | 0.200    |
| P2c_P3c_P4b_P5a_P6c   | P5            | carpisma_sayisi    | 0.000    | -0.200   |
| P2c_P3c_P4b_P5ab_P6c  | P5            | carpisma_sayisi    | 0.000    | -0.200   |
| P2c_P3c_P4b_P5abc_P6a | P6            | atama_kararliligi  | 0.000    | -0.060   |
| P2c_P3c_P4b_P5abc_P6a | P6            | churn_orani        | 0.000    | -0.020   |
| P2c_P3c_P4b_P5abc_P6b | P6            | churn_orani        | 0.033    | 0.000    |
| P2c_P3c_P4b_P5abc_P6b | P6            | karsilasma_sikligi | 0.000    | -0.100   |
| P2c_P3c_P4b_P5b_P6c   | P5            | churn_orani        | 0.000    | -0.031   |
| P2c_P3c_P4b_P5b_P6c   | P5            | carpisma_sayisi    | 0.000    | -0.300   |
| P2c_P3c_P4b_P5bc_P6c  | P5            | churn_orani        | 0.000    | -0.031   |
| P2c_P3c_P4b_P5c_P6c   | P5            | churn_orani        | 0.000    | -0.031   |
| P2c_P3c_P4c_P5abc_P6c | P4            | atama_kararliligi  | 0.000    | 0.100    |
| P2c_P3c_P4c_P5abc_P6c | P4            | churn_orani        | 0.000    | 0.019    |
| P2c_P3c_P4c_P5abc_P6c | P4            | carpisma_sayisi    | 0.000    | 0.100    |


## 3. N-Duyarlilik Skoru (Bolum 6)

`|N=5 degeri − N=3 degeri| / N=3 degeri`, kombinasyon basina.

`-` isareti, N=3 degerinin SIFIR oldugunu ve oransal degisimin tanimsiz kaldigini gosterir. Bu kendi basina bir bulgudur: metrik N=3'te hic hareket etmiyor, N=5'te ediyor (orn. churn orani -- takas mekanizmasi N=3'te hic tetiklenmiyor).

| kombinasyon_id         | gorev_tamamlama_suresi | atama_kararliligi | churn_orani | kapsama_dengesizligi | karsilasma_sikligi | iletisim_yuku | tick_maliyeti | carpisma_sayisi | ortalama_duyarlilik |
| ---------------------- | ---------------------- | ----------------- | ----------- | -------------------- | ------------------ | ------------- | ------------- | --------------- | ------------------- |
| P2c_P3b_P4b_P5abc_P6c  | 0.330                  | 0.236             | -           | -                    | 2.182              | 2.039         | 0.330         | 6.000           | 1.853               |
| P2c_P3c_P4c_P5abc_P6c  | 0.370                  | 0.127             | -           | -                    | 2.261              | 1.338         | 0.370         | 2.000           | 1.078               |
| P2c_P3c_P4b_P5abc_P6a  | 0.371                  | 0.345             | -           | -                    | 2.286              | 1.474         | 0.371         | 1.500           | 1.058               |
| P2c_P3a_P4b_P5abc_P6c  | 0.330                  | 0.250             | -           | -                    | 1.302              | 1.094         | 0.330         | 3.000           | 1.051               |
| P2c_P3c_P4b_P5abcd_P6c | 0.369                  | 0.264             | -           | -                    | 2.042              | 1.288         | 0.369         | 1.500           | 0.972               |
| P2c_P3c_P4b_P5ac_P6c   | 0.369                  | 0.264             | -           | -                    | 2.042              | 1.282         | 0.369         | 1.500           | 0.971               |
| P2c_P3c_P4b_P5abc_P6c  | 0.369                  | 0.264             | -           | -                    | 2.042              | 1.282         | 0.369         | 1.500           | 0.971               |
| P2c_P3c_P4b_P5c_P6c    | 0.419                  | 0.400             | -           | -                    | 2.091              | 2.070         | 0.419         | 0.333           | 0.955               |
| P2c_P3c_P4b_P5bc_P6c   | 0.419                  | 0.400             | -           | -                    | 2.091              | 2.070         | 0.419         | 0.333           | 0.955               |
| P2c_P3c_P4a_P5abc_P6c  | 0.370                  | 0.127             | -           | -                    | 2.261              | 0.066         | 0.370         | 2.500           | 0.949               |
| P2c_P3c_P4b_P5ab_P6c   | 0.336                  | 0.057             | -           | -                    | 2.190              | 2.149         | 0.336         | 0.500           | 0.928               |
| P2c_P3c_P4b_P5a_P6c    | 0.336                  | 0.057             | -           | -                    | 2.190              | 2.149         | 0.336         | 0.500           | 0.928               |
| P2a_P3c_P4b_P5abc_P6c  | 0.336                  | 0.400             | 0.647       | -                    | 2.208              | 0.471         | 0.336         | 2.000           | 0.914               |
| P2b_P3c_P4b_P5abc_P6c  | 0.366                  | 0.200             | 0.139       | -                    | 2.318              | 1.251         | 0.366         | 1.500           | 0.877               |
| P2c_P3c_P4b_P5abc_P6b  | 0.390                  | 0.300             | 0.066       | -                    | 2.000              | 1.238         | 0.390         | 1.500           | 0.840               |
| P2c_P3c_P4b_P5b_P6c    | 0.000                  | 0.400             | -           | -                    | 2.333              | 2.278         | 0.000         | 0.000           | 0.835               |


## 4. Hicbir Olcekte Etki Gostermeyen Eksenler

| degisen_eksen | olculen | etkisiz | etkisiz_orani |
| ------------- | ------- | ------- | ------------- |
| P5            | 56      | 22      | 0.393         |
| P6            | 16      | 4       | 0.250         |
| P2            | 16      | 3       | 0.188         |
| P4            | 16      | 2       | 0.125         |
| P3            | 16      | 2       | 0.125         |
