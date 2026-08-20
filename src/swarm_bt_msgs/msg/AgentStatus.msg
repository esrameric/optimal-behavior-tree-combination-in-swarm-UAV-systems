# Bir drone'un karsilasma aninda paylastigi durum bilgisi.
# Plan Bolum 2.2: durum_bilgisi_degis() -> kalan_alan_yuzdesi, batarya,
# oncelik_puani karsilikli paylasilir.

builtin_interfaces/Time stamp

int32 agent_id
bool alive

# Konum [m] (2D; Faz 1 kapsami 2D, Faz 2'de z gormezden gelinir)
float64 x
float64 y

# Kalan alan orani [0,1]: bolgesindeki taranmamis hucre / toplam hucre
float64 remaining_area_ratio

# Kalan batarya orani [0,1]
float64 battery

# Muzakere onceligi; buyuk olan once teklif eder
float64 priority_score

# Bolgesindeki toplam ve kalan hucre sayisi
int32 region_cells
int32 remaining_cells
