# Iki drone'un iletisim menziline girmesiyle dogan karsilasma olayi.
# Plan Bolum 2.1 adim 5 ve Bolum 6 "karsilasma sikligi" metrigi.
#
# Olay YUKSELEN KENARDA uretilir: menzil icinde kalindigi surece tekrar
# yayinlanmaz (bkz. EncounterDetector histerezisi).

builtin_interfaces/Time stamp

int32 agent_a
int32 agent_b

# Olay anindaki ikili mesafe [m]
float64 distance

# Kullanilan iletisim menzili [m]
float64 comm_range
