#!/usr/bin/env bash
# Faz 2 kampanyasi - plan Bolum 5/Faz 2.
#
# Finalist kombinasyonlari N=3 ve N=5 ile HEM Gazebo'da HEM kod-seviyesinde
# koşar ve sonuclari tek bir CSV'ye yazar. Iki faz ayni deney dosyasini ve ayni
# tohumu kullanir; kalkis konumlari da esitlenir (launch, konumlari
# simulatorden sorar). Boylece aradaki fark yalnizca POZISYON KAYNAGINDAN
# gelir -- plan Bolum 7'nin dayandigi ayrim budur.
#
# Kullanim:
#   ./tools/run_phase2.sh [tekrar_sayisi] [cikti.csv]
# NOT: set -u kullanilmiyor -- ROS2 setup.bash tanimsiz degiskenlere dokunuyor
# ve -u ile aninda hata veriyor.
set -o pipefail

REPETITIONS="${1:-5}"
OUTPUT="${2:-experiments/phase2_gazebo.csv}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${REPO_ROOT}"

# Plan Bolum 5/Faz 2: her finalist icin >= 5 tekrar.
if [[ "${REPETITIONS}" -lt 5 ]]; then
  echo "UYARI: tekrar sayisi ${REPETITIONS} < 5. Plan en az 5 tekrar istiyor." >&2
fi

# shellcheck disable=SC1091
source /opt/ros/humble/setup.bash
# shellcheck disable=SC1091
source install/setup.bash

CONFIG_DIR="install/swarm_bt_bringup/share/swarm_bt_bringup/config"
SHARE_CONFIG_DIR="src/swarm_bt_bringup/config"

# Faz 2 senaryosu: kucultulmus alan (Gazebo gercek zamanli kostugu icin).
AREA=120
RCOMM=18

finalists=(
  "P2c_P3c_P4b_P5abc_P6c"   # baseline
  "P2c_P3b_P4b_P5abc_P6c"   # Contract Net  - en yuksek N-duyarlilik
  "P2c_P3c_P4c_P5abc_P6c"   # olay-gudumlu BT
  "P2c_P3c_P4b_P5abc_P6a"   # periyodik yoklama
  "P2a_P3c_P4b_P5abc_P6c"   # tam merkezi - plan Bolum 5/Faz 3 bunu soruyor
)

echo "deney_id,kombinasyon_id,faz,N,tohum,gorev_suresi,tick,kapsama_tamam,karsilasma,takas,devralinan,churn_orani,atama_kararliligi,carpisma" > "${OUTPUT}"

extract() {  # $1 cikti metni, $2 alan etiketi
  echo "$1" | grep -m1 "$2" | awk -F: '{print $2}' | awk '{print $1}'
}

for combination in "${finalists[@]}"; do
  for n in 3 5; do
    experiment_id="${combination}_N${n}"
    config_name="experiment_phase2_${combination}_N${n}.yaml"

    # Faz 2 senaryosuyla config uret (alan ve r_comm olcekli).
    ros2 run swarm_bt_core experiment_config_tool \
      --id "${experiment_id}" --area "${AREA}" --r-comm "${RCOMM}" \
      > "${SHARE_CONFIG_DIR}/${config_name}" 2>/dev/null
    # Faz 2'de fizik motoru tam duramaz; tolerans buyutulur.
    sed -i 's/  waypoint_tolerance: .*/  waypoint_tolerance: 1.5/' \
      "${SHARE_CONFIG_DIR}/${config_name}"
    sed -i 's/  safety_radius: .*/  safety_radius: 3/' \
      "${SHARE_CONFIG_DIR}/${config_name}"
    cp "${SHARE_CONFIG_DIR}/${config_name}" "${CONFIG_DIR}/${config_name}"

    for ((seed = 0; seed < REPETITIONS; ++seed)); do
      # --- kod-seviyesi (Faz 1 motoru) ---
      code_output="$(timeout 180 ros2 run swarm_bt_sim sim_runner \
        --config "${CONFIG_DIR}/${config_name}" --seed "${seed}" 2>/dev/null)"
      echo "${experiment_id},${combination},kod,${n},${seed},\
$(extract "${code_output}" 'gorev suresi'),\
$(extract "${code_output}" 'tick maliyeti'),\
$([[ "$(extract "${code_output}" 'kapsama tamam')" == "evet" ]] && echo 1 || echo 0),\
$(extract "${code_output}" 'karsilasma sikligi'),\
$(extract "${code_output}" 'kabul edilen takas'),\
$(extract "${code_output}" 'devralinan hucre'),\
$(extract "${code_output}" 'churn orani'),\
$(extract "${code_output}" 'atama kararliligi'),\
$(extract "${code_output}" 'kapsama dengesizligi')" >> "${OUTPUT}"

      # --- Gazebo ---
      gz_log="$(mktemp)"
      timeout 240 ros2 launch swarm_bt_bringup phase2_gazebo.launch.py \
        n_agents:="${n}" config:="${config_name}" seed:="${seed}" \
        > "${gz_log}" 2>&1
      summary="$(grep -m1 'Koşu bitti' "${gz_log}" || true)"
      rm -f "${gz_log}"

      if [[ -z "${summary}" ]]; then
        echo "${experiment_id},${combination},gazebo,${n},${seed},,,0,,,,,," >> "${OUTPUT}"
        echo "  [gazebo] ${experiment_id} tohum ${seed}: TAMAMLANMADI" >&2
        continue
      fi

      # "sure=36.0 s tick=360 kapsama=tamam karsilasma=0 ..." bicimini ayristir.
      field() { echo "${summary}" | grep -o "$1=[^ ]*" | head -1 | cut -d= -f2; }
      echo "${experiment_id},${combination},gazebo,${n},${seed},\
$(field sure),$(field tick),\
$([[ "$(field kapsama)" == "tamam" ]] && echo 1 || echo 0),\
$(field karsilasma),$(field takas),$(field devralinan),\
$(field churn),$(field kararlilik),$(field carpisma)" >> "${OUTPUT}"
      echo "  [gazebo] ${experiment_id} tohum ${seed}: $(field sure) s"
    done
  done
done

echo "Faz 2 kampanyasi bitti: ${OUTPUT} ($(($(wc -l < "${OUTPUT}") - 1)) satir)"
