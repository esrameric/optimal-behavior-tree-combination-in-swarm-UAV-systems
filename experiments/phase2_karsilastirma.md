# Faz 1 ↔ Faz 2 Karşılaştırması

- Kombinasyon sayısı: 5
- Ölçek değerleri: N=3, N=5
- Toplam koşu: 100

## 1. Tamamlanma ve Tekrar Denetimi

| faz    | kombinasyon_id        | N | tekrar | tamamlanma_orani |
| ------ | --------------------- | - | ------ | ---------------- |
| gazebo | P2a_P3c_P4b_P5abc_P6c | 3 | 5      | 1.000            |
| gazebo | P2a_P3c_P4b_P5abc_P6c | 5 | 5      | 0.800            |
| gazebo | P2c_P3b_P4b_P5abc_P6c | 3 | 5      | 1.000            |
| gazebo | P2c_P3b_P4b_P5abc_P6c | 5 | 5      | 1.000            |
| gazebo | P2c_P3c_P4b_P5abc_P6a | 3 | 5      | 1.000            |
| gazebo | P2c_P3c_P4b_P5abc_P6a | 5 | 5      | 1.000            |
| gazebo | P2c_P3c_P4b_P5abc_P6c | 3 | 5      | 1.000            |
| gazebo | P2c_P3c_P4b_P5abc_P6c | 5 | 5      | 1.000            |
| gazebo | P2c_P3c_P4c_P5abc_P6c | 3 | 5      | 1.000            |
| gazebo | P2c_P3c_P4c_P5abc_P6c | 5 | 5      | 1.000            |
| kod    | P2a_P3c_P4b_P5abc_P6c | 3 | 5      | 1.000            |
| kod    | P2a_P3c_P4b_P5abc_P6c | 5 | 5      | 1.000            |
| kod    | P2c_P3b_P4b_P5abc_P6c | 3 | 5      | 1.000            |
| kod    | P2c_P3b_P4b_P5abc_P6c | 5 | 5      | 1.000            |
| kod    | P2c_P3c_P4b_P5abc_P6a | 3 | 5      | 1.000            |
| kod    | P2c_P3c_P4b_P5abc_P6a | 5 | 5      | 1.000            |
| kod    | P2c_P3c_P4b_P5abc_P6c | 3 | 5      | 1.000            |
| kod    | P2c_P3c_P4b_P5abc_P6c | 5 | 5      | 1.000            |
| kod    | P2c_P3c_P4c_P5abc_P6c | 3 | 5      | 1.000            |
| kod    | P2c_P3c_P4c_P5abc_P6c | 5 | 5      | 1.000            |


## 2. Karşılaşma Sıklığı Ölçekle Artıyor mu

Plan Bölüm 5/Faz 2: "N=5'te beklenen artışı doğrula, bu confound'u raporda açıkça belirt".

| faz    | kombinasyon_id        | karsilasma_N3 | karsilasma_N5 | artis | artis_katsayisi | artti_mi |
| ------ | --------------------- | ------------- | ------------- | ----- | --------------- | -------- |
| gazebo | P2a_P3c_P4b_P5abc_P6c | 0.800         | 3.250         | 2.450 | 4.062           | True     |
| gazebo | P2c_P3b_P4b_P5abc_P6c | 0.400         | 3.200         | 2.800 | 8.000           | True     |
| gazebo | P2c_P3c_P4b_P5abc_P6a | 0.200         | 2.600         | 2.400 | 13.000          | True     |
| gazebo | P2c_P3c_P4b_P5abc_P6c | 0.800         | 3.600         | 2.800 | 4.500           | True     |
| gazebo | P2c_P3c_P4c_P5abc_P6c | 1.000         | 4.400         | 3.400 | 4.400           | True     |
| kod    | P2a_P3c_P4b_P5abc_P6c | 0.800         | 3.600         | 2.800 | 4.500           | True     |
| kod    | P2c_P3b_P4b_P5abc_P6c | 0.400         | 3.200         | 2.800 | 8.000           | True     |
| kod    | P2c_P3c_P4b_P5abc_P6a | 0.200         | 2.400         | 2.200 | 12.000          | True     |
| kod    | P2c_P3c_P4b_P5abc_P6c | 0.800         | 3.600         | 2.800 | 4.500           | True     |
| kod    | P2c_P3c_P4c_P5abc_P6c | 1.400         | 3.800         | 2.400 | 2.714           | True     |


## 3. İki Fazın N-Duyarlılığı Aynı Yönde mi

Etkili 31 ölçümün 23 tanesinde iki faz aynı yönde. Aşağıdaki tabloda **yalnızca uyuşmayanlar** listelenir.

| kombinasyon_id        | metrik            | delta_kod | delta_gazebo | ayni_yon |
| --------------------- | ----------------- | --------- | ------------ | -------- |
| P2a_P3c_P4b_P5abc_P6c | carpisma          | 0.000     | 1.050        | False    |
| P2c_P3b_P4b_P5abc_P6c | carpisma          | 0.000     | 0.800        | False    |
| P2c_P3c_P4b_P5abc_P6a | takas             | 0.000     | 0.200        | False    |
| P2c_P3c_P4b_P5abc_P6a | churn_orani       | 0.000     | 0.040        | False    |
| P2c_P3c_P4b_P5abc_P6a | atama_kararliligi | 0.000     | 0.080        | False    |
| P2c_P3c_P4b_P5abc_P6a | carpisma          | 0.000     | 1.000        | False    |
| P2c_P3c_P4b_P5abc_P6c | carpisma          | 0.000     | 0.800        | False    |
| P2c_P3c_P4c_P5abc_P6c | carpisma          | 0.000     | 1.400        | False    |


## 4. Model Doğruluğu (bağıl sapma)

Kod-seviyesi model Gazebo'yu ne kadar iyi tahmin ediyor?

| kombinasyon_id        | N | gorev_suresi_bagil_sapma | tick_bagil_sapma | karsilasma_bagil_sapma | takas_bagil_sapma | churn_orani_bagil_sapma | atama_kararliligi_bagil_sapma | carpisma_bagil_sapma |
| --------------------- | - | ------------------------ | ---------------- | ---------------------- | ----------------- | ----------------------- | ----------------------------- | -------------------- |
| P2a_P3c_P4b_P5abc_P6c | 3 | 0.041                    | 0.041            | 0.000                  | 0.000             | 0.000                   | 0.004                         | -                    |
| P2a_P3c_P4b_P5abc_P6c | 5 | 0.046                    | 0.046            | 0.097                  | -                 | -                       | -                             | -                    |
| P2c_P3b_P4b_P5abc_P6c | 3 | 0.010                    | 0.010            | 0.000                  | -                 | -                       | -                             | -                    |
| P2c_P3b_P4b_P5abc_P6c | 5 | 0.032                    | 0.032            | 0.000                  | -                 | -                       | -                             | -                    |
| P2c_P3c_P4b_P5abc_P6a | 3 | 0.037                    | 0.037            | 0.000                  | -                 | -                       | -                             | -                    |
| P2c_P3c_P4b_P5abc_P6a | 5 | 0.072                    | 0.072            | 0.083                  | -                 | -                       | -                             | -                    |
| P2c_P3c_P4b_P5abc_P6c | 3 | 0.045                    | 0.045            | 0.000                  | 0.000             | 0.000                   | 0.004                         | -                    |
| P2c_P3c_P4b_P5abc_P6c | 5 | 0.062                    | 0.062            | 0.000                  | 0.000             | 0.009                   | 0.000                         | -                    |
| P2c_P3c_P4c_P5abc_P6c | 3 | 0.023                    | 0.023            | 0.286                  | 1.000             | 1.000                   | 1.000                         | -                    |
| P2c_P3c_P4c_P5abc_P6c | 5 | 0.165                    | 0.165            | 0.158                  | 0.750             | 0.163                   | 0.750                         | -                    |

