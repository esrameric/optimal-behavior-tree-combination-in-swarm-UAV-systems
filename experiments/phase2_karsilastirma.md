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

