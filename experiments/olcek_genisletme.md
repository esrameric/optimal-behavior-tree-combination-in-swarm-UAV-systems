# Ölçek Genişletme Kontrolleri — Bölüm 9

**Plan Bölüm 9, iki isteğe bağlı genişletme:**

1. *"Mission alanını N ile orantılı büyüterek (drone başına düşen alan sabit
   kalacak şekilde) saf ölçeklenebilirliği (karşılaşma sıklığı etkisi olmadan)
   ayrıca test edebilirsin — bu, confound'u ortadan kaldıran temiz bir kontrol
   grubu olur."*
2. *"Eğer N=3→N=5 arasında ilginç bir trend görürsen, bunun devam edip
   etmediğini görmek için üçüncü bir N değeri eklemeyi düşünebilirsin."*

**İkisi de yapıldı.** Ham veri: [`olcek_genisletme.csv`](olcek_genisletme.csv)
Araç: `ros2 run swarm_bt_sim scale_extension --repetitions 10`
Ölçekler: **N ∈ {3, 5, 7, 10}**, arızalı senaryo, 10 tekrar.

## 1. Trend devam ediyor — ama yavaşlayarak

Sabit alan (bu çalışmanın ana kurulumu), atama kararlılığı:

| N | hücre/ajan | karşılaşma | görev süresi (s) | **atama kararlılığı** |
|---|---|---|---|---|
| 3 | 133.3 | 2.4 | 468.73 | **0.733** |
| 5 | 80.0 | 7.3 | 295.76 | **0.540** |
| 7 | 57.1 | 16.8 | 246.78 | **0.443** |
| 10 | 40.0 | 27.3 | 197.69 | **0.410** |

Faz 3'ün bulgusu (ajan başına atama değişikliği ölçekle azalıyor = sistem daha
kararlı) **N=10'a kadar monoton olarak sürüyor**. Ama azalma hızı düşüyor:
−0.193, −0.097, −0.033. Etki **doyuma gidiyor**; N=7'den sonra ek kazanç küçük.

Görev süresi de aynı şekilde doyuyor: 468 → 296 → 247 → 198 s.

## 2. Saf ölçeklenebilirlik — N=5'ten sonra TERSİNE dönüyor

Orantılı alan (drone başına düşen alan sabit, hücre/ajan ≈ 125-133 ✓):

| N | alan kenarı (m) | r_comm (m) | hücre/ajan | görev süresi (s) | kapsama tamamlanma |
|---|---|---|---|---|---|
| 3 | 400.0 | 60.0 | 133.3 | **468.73** | 1.00 |
| 5 | 516.4 | 77.5 | 125.0 | **453.18** | 1.00 |
| 7 | 611.0 | 91.7 | 128.6 | **656.22** | 0.90 |
| 10 | 730.3 | 109.5 | 129.6 | **658.06** | 0.90 |

**İş yükü ajan başına sabitken, drone eklemek görevi N=5'ten sonra
YAVAŞLATIYOR.** N=3→N=5'te küçük bir iyileşme (−15.6 s) var, ama N=7'de
+203 s'lik bir kötüleşme geliyor ve koşuların %10'u kapsamayı tamamlayamıyor.

Sebep iletişim yükünde görünüyor:

| N | iletişim yükü (sabit alan) | iletişim yükü (orantılı alan) |
|---|---|---|
| 3 | 142.8 | 142.8 |
| 5 | 325.9 | 613.8 |
| 7 | 652.3 | 1579.0 |
| 10 | 1036.4 | **4318.1** |

Orantılı alanda iletişim yükü N=3→N=10 arasında **30 kat** artıyor. Koordinasyon
maliyeti, ajan başına sabit tutulan işin kazancını yiyor.

**Bu, çalışmanın ölçeklenebilirlik sınırıdır:** bu koordinasyon mimarisi
(P2c tam dağıtık + P4b özdeş dağıtık BT) N≈5'e kadar ölçekleniyor, sonrasında
koordinasyon maliyeti baskın hale geliyor.

## 3. Orantılı alan confound'u TAM kaldırmıyor

Planın "confound'u ortadan kaldıran temiz bir kontrol grubu" beklentisi kısmen
karşılanıyor. Karşılaşma sayısı orantılı alanda da artıyor (2.4 → 37.6), hatta
sabit alandakinden **daha fazla**.

Sebep: orantılı alan, ajan başına **işi** sabit tutuyor ama **çift sayısını**
tutmuyor. Çift sayısı `C(N,2)` ile, yani N² gibi büyüyor; alan yalnızca N ile
büyüyor. Sonuçta ajan başına düşen komşu sayısı artıyor.

Karşılaşma sıklığını gerçekten sabitleyen kontrol
[`confound_kontrol.md`](confound_kontrol.md)'dedir (r_comm'u ölçeğe göre
ayarlayarak). İki kontrol farklı şeyleri sabitliyor:

| kontrol | sabit tutulan | ölçtüğü |
|---|---|---|
| orantılı alan | ajan başına **iş yükü** | saf ölçeklenebilirlik (koordinasyon maliyeti dahil) |
| eşitli r_comm | **karşılaşma sıklığı** | N'in koordinasyondan arındırılmış etkisi |

## Sonuç

- Faz 3'ün "ölçekle daha kararlı" bulgusu **N=10'a kadar geçerli**, doyuma
  giderek.
- Ama **saf ölçeklenebilirlik N≈5'te sınırlanıyor**: iş yükü sabitken drone
  eklemek görevi yavaşlatıyor ve iletişim yükü 30 kat artıyor.
- İki bulgu çelişmiyor: sürü **daha kararlı** ama **daha verimsiz** hale geliyor.
