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
| P2c_P3a_P4b_P5abc_P6c | P3            | churn_orani            | -0.010   | 0.043    |
| P2c_P3a_P4b_P5abc_P6c | P3            | carpisma_sayisi        | -0.100   | 0.100    |
| P2c_P3b_P4b_P5abc_P6c | P3            | gorev_tamamlama_suresi | -18.200  | 13.530   |
| P2c_P3b_P4b_P5abc_P6c | P3            | tick_maliyeti          | -182.000 | 135.300  |
| P2c_P3b_P4b_P5abc_P6c | P3            | carpisma_sayisi        | -0.100   | 0.200    |
| P2c_P3c_P4b_P5abc_P6a | P6            | iletisim_yuku          | -1.800   | 48.400   |


## 2. Eksen Bazinda Ozet

| degisen_eksen | olculen | yon_catismasi | tek_tarafli | etkisiz |
| ------------- | ------- | ------------- | ----------- | ------- |
| P3            | 16      | 5             | 3           | 2       |
| P6            | 16      | 1             | 2           | 9       |
| P2            | 16      | 0             | 1           | 9       |
| P4            | 16      | 0             | 0           | 16      |
| P5            | 56      | 0             | 3           | 35      |


### 2b. Olcek Esigi Gosteren Olcumler

Etki yalnizca bir olcekte ortaya cikiyor; parametre o esigin altinda/ustunde davranis degistiriyor.

| kombinasyon_id        | degisen_eksen | metrik            | delta_N3 | delta_N5 |
| --------------------- | ------------- | ----------------- | -------- | -------- |
| P2b_P3c_P4b_P5abc_P6c | P2            | iletisim_yuku     | 0.000    | -0.100   |
| P2c_P3a_P4b_P5abc_P6c | P3            | atama_kararliligi | 0.000    | 0.100    |
| P2c_P3b_P4b_P5abc_P6c | P3            | atama_kararliligi | 0.000    | -0.020   |
| P2c_P3b_P4b_P5abc_P6c | P3            | churn_orani       | 0.000    | 0.035    |
| P2c_P3c_P4b_P5a_P6c   | P5            | carpisma_sayisi   | 0.000    | -0.100   |
| P2c_P3c_P4b_P5ab_P6c  | P5            | carpisma_sayisi   | 0.000    | -0.100   |
| P2c_P3c_P4b_P5abc_P6a | P6            | atama_kararliligi | 0.000    | -0.020   |
| P2c_P3c_P4b_P5abc_P6a | P6            | churn_orani       | 0.000    | -0.004   |
| P2c_P3c_P4b_P5b_P6c   | P5            | carpisma_sayisi   | 0.000    | -0.100   |


## 3. N-Duyarlilik Skoru (Bolum 6)

`|N=5 degeri − N=3 degeri| / N=3 degeri`, kombinasyon basina.

| kombinasyon_id         | gorev_tamamlama_suresi | atama_kararliligi | churn_orani | kapsama_dengesizligi | karsilasma_sikligi | iletisim_yuku | tick_maliyeti | carpisma_sayisi | ortalama_duyarlilik |
| ---------------------- | ---------------------- | ----------------- | ----------- | -------------------- | ------------------ | ------------- | ------------- | --------------- | ------------------- |
| P2c_P3b_P4b_P5abc_P6c  | 0.312                  | 0.373             | 0.387       | -                    | 1.909              | 1.637         | 0.312         | 5.000           | 1.418               |
| P2a_P3c_P4b_P5abc_P6c  | 0.352                  | 0.400             | 0.661       | -                    | 2.091              | 2.958         | 0.352         | 1.000           | 1.116               |
| P2c_P3a_P4b_P5abc_P6c  | 0.361                  | 0.209             | 0.688       | -                    | 1.190              | 0.526         | 0.361         | 4.000           | 1.048               |
| P2c_P3c_P4b_P5c_P6c    | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 2.043         | 0.368         | 1.000           | 0.886               |
| P2c_P3c_P4b_P5bc_P6c   | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 2.043         | 0.368         | 1.000           | 0.886               |
| P2c_P3c_P4b_P5abc_P6b  | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 1.678         | 0.368         | 1.000           | 0.834               |
| P2c_P3c_P4b_P5b_P6c    | 0.336                  | 0.186             | 0.035       | -                    | 2.200              | 2.200         | 0.336         | 0.500           | 0.827               |
| P2c_P3c_P4b_P5abc_P6a  | 0.368                  | 0.373             | 0.086       | -                    | 2.136              | 1.323         | 0.368         | 1.000           | 0.808               |
| P2c_P3c_P4b_P5a_P6c    | 0.336                  | 0.186             | 0.035       | -                    | 2.200              | 1.976         | 0.336         | 0.500           | 0.795               |
| P2c_P3c_P4b_P5ab_P6c   | 0.336                  | 0.186             | 0.035       | -                    | 2.200              | 1.976         | 0.336         | 0.500           | 0.795               |
| P2c_P3c_P4b_P5abcd_P6c | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 1.010         | 0.368         | 1.000           | 0.739               |
| P2c_P3c_P4a_P5abc_P6c  | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 1.005         | 0.368         | 1.000           | 0.738               |
| P2c_P3c_P4b_P5ac_P6c   | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 1.005         | 0.368         | 1.000           | 0.738               |
| P2c_P3c_P4b_P5abc_P6c  | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 1.005         | 0.368         | 1.000           | 0.738               |
| P2c_P3c_P4c_P5abc_P6c  | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 1.005         | 0.368         | 1.000           | 0.738               |
| P2b_P3c_P4b_P5abc_P6c  | 0.368                  | 0.345             | 0.035       | -                    | 2.043              | 1.005         | 0.368         | 1.000           | 0.738               |


## 4. Hicbir Olcekte Etki Gostermeyen Eksenler

| degisen_eksen | olculen | etkisiz | etkisiz_orani |
| ------------- | ------- | ------- | ------------- |
| P4            | 16      | 16      | 1.000         |
| P5            | 56      | 35      | 0.625         |
| P2            | 16      | 9       | 0.562         |
| P6            | 16      | 9       | 0.562         |
| P3            | 16      | 2       | 0.125         |
