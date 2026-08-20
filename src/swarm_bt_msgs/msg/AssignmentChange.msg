# Bir drone'un atanmis alaninin degistigi an.
# Plan Bolum 6: "atama kararliligi" ve "churn orani" metrikleri bu olaylardan
# hesaplanir; her degisiklikte rosbag2'ye bir kayit duşer.

builtin_interfaces/Time stamp

int32 agent_id

# Degisikligin sebebi
uint8 REASON_AREA_SWAP=0        # alan takasi (Bolum 2.2)
uint8 REASON_ORPHAN_TAKEOVER=1  # arizali drone'dan devralma (Bolum 2.3)
uint8 REASON_JOINT_SCAN=2       # ortak tarama (Bolum 2.2)
uint8 REASON_INITIAL=3          # baslangic atamasi (Bolum 2.1 adim 2)
uint8 REASON_FAILURE=4          # ajanin kendisi arizalandi
uint8 reason

# Muzakere ortagi; yoksa -1
int32 peer_id

# Degisiklikten etkilenen hucre sayisi
int32 cells_changed

# Degisiklik sonrasi bolge buyuklugu ve kalan hucre sayisi
int32 region_cells
int32 remaining_cells

# Ajanin koşu boyunca kacinci atama degisikligi (atama kararliligi sayaci)
int32 change_index
