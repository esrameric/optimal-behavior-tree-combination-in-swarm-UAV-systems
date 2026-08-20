# Parametre Eksenlerinin Ölçek Duyarlılığı

Plan Bölüm 5/Faz 3: "Hangi parametrelerin N-duyarlılığı en yüksek/düşük olduğunu belirle." Ölçek: N=3 → N=5.

Her OFAT varyantı baseline'dan **tek bir eksende** ayrıldığı için, o varyantın N-duyarlılık skoru o eksene atfedilebilir.

## 1. Eksen Sıralaması

- **En duyarlı eksen: P3** (ortalama 1.452)
- **En az duyarlı eksen: P2** (ortalama 0.896)

| eksen | varyant | ortalama | en_yuksek | en_duşuk |
| ----- | ------- | -------- | --------- | -------- |
| P3    | 2       | 1.452    | 1.853     | 1.051    |
| P4    | 2       | 1.013    | 1.078     | 0.949    |
| P6    | 2       | 0.949    | 1.058     | 0.840    |
| P5    | 7       | 0.935    | 0.972     | 0.835    |
| P2    | 2       | 0.896    | 0.914     | 0.877    |


## 2. Seçenek Bazında

| eksen | secenek | secenek_adi                                                 | duyarlilik | kombinasyon_id         |
| ----- | ------- | ----------------------------------------------------------- | ---------- | ---------------------- |
| P3    | b       | Contract Net                                                | 1.853      | P2c_P3b_P4b_P5abc_P6c  |
| P4    | c       | olay-gudumlu BT                                             | 1.078      | P2c_P3c_P4c_P5abc_P6c  |
| P6    | a       | periyodik yoklama                                           | 1.058      | P2c_P3c_P4b_P5abc_P6a  |
| P3    | a       | statik esit bolme                                           | 1.051      | P2c_P3a_P4b_P5abc_P6c  |
| P5    | abcd    | dogrudan mesaj + stigmerji + intent yayini + kulak misafiri | 0.972      | P2c_P3c_P4b_P5abcd_P6c |
| P5    | ac      | dogrudan mesaj + intent yayini                              | 0.971      | P2c_P3c_P4b_P5ac_P6c   |
| P5    | c       | intent yayini                                               | 0.955      | P2c_P3c_P4b_P5c_P6c    |
| P5    | bc      | stigmerji + intent yayini                                   | 0.955      | P2c_P3c_P4b_P5bc_P6c   |
| P4    | a       | tek merkezi BT                                              | 0.949      | P2c_P3c_P4a_P5abc_P6c  |
| P5    | ab      | dogrudan mesaj + stigmerji                                  | 0.928      | P2c_P3c_P4b_P5ab_P6c   |
| P5    | a       | dogrudan mesaj                                              | 0.928      | P2c_P3c_P4b_P5a_P6c    |
| P2    | a       | tam merkezi                                                 | 0.914      | P2a_P3c_P4b_P5abc_P6c  |
| P2    | b       | hiyerarsik hibrit                                           | 0.877      | P2b_P3c_P4b_P5abc_P6c  |
| P6    | b       | her tick kontrol                                            | 0.840      | P2c_P3c_P4b_P5abc_P6b  |
| P5    | b       | stigmerji                                                   | 0.835      | P2c_P3c_P4b_P5b_P6c    |


## 3. Merkezi mi Dağıtık mı — planın açık sorusu

"Tam merkezi mimari N arttıkça mı bozuluyor, tam dağıtık mı daha dayanıklı çıkıyor?"

| P2 | mimari            | gorev_tamamlama_suresi_N3 | gorev_tamamlama_suresi_N5 | delta    | baseline_farki_N3 | baseline_farki_N5 |
| -- | ----------------- | ------------------------- | ------------------------- | -------- | ----------------- | ----------------- |
| a  | tam merkezi       | 464.870                   | 308.500                   | -156.370 | -3.860            | 12.740            |
| b  | hiyerarsik hibrit | 458.340                   | 290.520                   | -167.820 | -10.390           | -5.240            |
| c  | tam dagitik       | 468.730                   | 295.760                   | -172.970 | 0.000             | 0.000             |


**Cevap:** tam merkezi mimari N=3'te dağıtıktan **iyi** (-3.86 s), N=5'te **kötü** (+12.74 s). Merkezi koordinasyon ölçekle bozuluyor; tam dağıtık daha dayanıklı.

