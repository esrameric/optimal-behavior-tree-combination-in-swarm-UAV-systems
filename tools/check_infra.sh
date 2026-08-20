#!/usr/bin/env bash
# Faz 0 altyapi dogrulamasi - plan Bolum 5/Faz 0.
#
# Plan, altyapinin (ROS2, BT.CPP, Groot2, Gazebo+SITL, rosbag2) yeniden
# kurulmamasini, mevcut kurulumun kullanilmasini soyluyor. Bu script mevcut
# durumu tek yerden raporlar: neyin hazir oldugunu, neyin eksik oldugunu ve
# eksik olanin hangi asamayi bloke ettigini gosterir.
#
# Cikis kodu: 0 = Faz 1 icin gereken her sey hazir, 1 = zorunlu bir parca eksik.
set -uo pipefail

ROS_DISTRO_EXPECTED="humble"
ROS_PREFIX="${ROS_PREFIX:-/opt/ros/${ROS_DISTRO_EXPECTED}}"

blocking_missing=0
optional_missing=0

report() {
  # $1 durum (VAR/YOK), $2 ad, $3 ayrinti, $4 zorunlu mu (evet/hayir)
  printf '  [%-3s] %-22s %s\n' "$1" "$2" "$3"
  if [[ "$1" == "YOK" ]]; then
    if [[ "$4" == "evet" ]]; then
      blocking_missing=$((blocking_missing + 1))
    else
      optional_missing=$((optional_missing + 1))
    fi
  fi
}

echo "Faz 0 altyapi dogrulamasi"
echo "========================="
echo
echo "Faz 1 (kod-seviyesi tarama) icin ZORUNLU:"

if [[ -d "${ROS_PREFIX}" ]]; then
  report VAR "ROS2 ${ROS_DISTRO_EXPECTED}" "${ROS_PREFIX}" evet
else
  report YOK "ROS2 ${ROS_DISTRO_EXPECTED}" "bulunamadi: ${ROS_PREFIX}" evet
fi

btcpp_version="$(dpkg-query -W -f='${Version}' ros-humble-behaviortree-cpp 2>/dev/null || true)"
if [[ -n "${btcpp_version}" ]]; then
  report VAR "BehaviorTree.CPP" "surum ${btcpp_version}" evet
else
  report YOK "BehaviorTree.CPP" "sudo apt install ros-humble-behaviortree-cpp" evet
fi

if command -v colcon > /dev/null 2>&1; then
  report VAR "colcon" "$(command -v colcon)" evet
else
  report YOK "colcon" "sudo apt install python3-colcon-common-extensions" evet
fi

if python3 -c "import pandas" > /dev/null 2>&1; then
  report VAR "pandas (analiz)" "$(python3 -c 'import pandas; print(pandas.__version__)')" evet
else
  report YOK "pandas (analiz)" "pip install pandas" evet
fi

echo
echo "Faz 2 (Gazebo dogrulamasi) icin ZORUNLU:"

# NOT: "dpkg -l | grep" kullanilmiyor -- dpkg -l ciktisini terminal genisligine
# gore KIRPIYOR ve boru icinde uzun paket adlari kesiliyor. dpkg-query kirpmaz.
gz_version="$(dpkg-query -W -f='${Version}' ros-humble-ros-gzharmonic 2>/dev/null || true)"
if [[ -n "${gz_version}" ]]; then
  report VAR "Gazebo Harmonic" "ros-humble-ros-gzharmonic ${gz_version}" evet
else
  report YOK "Gazebo Harmonic" "sudo apt install ros-humble-ros-gzharmonic" evet
fi

# Plan Bolum 0 uyarisi: Fortress paketleri Harmonic ile catisir.
fortress_status="$(dpkg-query -W -f='${Status}' ros-humble-ros-gz 2>/dev/null || true)"
if [[ "${fortress_status}" == *"install ok installed"* ]]; then
  report YOK "Fortress catismasi" "ros-humble-ros-gz kurulu! sudo apt remove ros-humble-ros-gz" evet
else
  report VAR "Fortress catismasi" "yok (Harmonic ile catisan paket kurulu degil)" evet
fi

px4_dir="${PX4_DIR:-${HOME}/PX4-Autopilot}"
if [[ -d "${px4_dir}" ]]; then
  report VAR "PX4-Autopilot" "${px4_dir}" evet
else
  report YOK "PX4-Autopilot" "git clone https://github.com/PX4/PX4-Autopilot.git" evet
fi

if [[ -d "${ROS_PREFIX}/share/rosbag2_cpp" ]]; then
  report VAR "rosbag2" "${ROS_PREFIX}/share/rosbag2_cpp" evet
else
  report YOK "rosbag2" "sudo apt install ros-humble-rosbag2" evet
fi

echo
echo "OPSIYONEL (eksikligi hicbir fazi bloke etmez):"

groot="$(command -v Groot2 2>/dev/null || command -v groot2 2>/dev/null || true)"
if [[ -n "${groot}" ]]; then
  report VAR "Groot2" "${groot}" hayir
else
  report YOK "Groot2" "BT gorsellestirme GUI'si (AppImage); BT.CPP tarafi hazir" hayir
fi

# matplotlib "kurulu ama ice aktarilamiyor" olabilir (numpy surum catismasi),
# bu yuzden varligi degil ICE AKTARILABILIRLIGI kontrol edilir.
if python3 -c "import matplotlib; matplotlib.use('Agg')" > /dev/null 2>&1; then
  report VAR "matplotlib" "$(python3 -c 'import matplotlib; print(matplotlib.__version__)')" hayir
elif python3 -c "import importlib.util,sys; sys.exit(0 if importlib.util.find_spec('matplotlib') else 1)" > /dev/null 2>&1; then
  report YOK "matplotlib" "kurulu ama ice aktarilamiyor (numpy surum catismasi olabilir)" hayir
else
  report YOK "matplotlib" "Faz 3 grafikleri icin gerekir: pip install matplotlib" hayir
fi

echo
echo "-------------------------------------------------"
if [[ ${blocking_missing} -eq 0 ]]; then
  echo "SONUC: zorunlu parcalarin hepsi hazir (${optional_missing} opsiyonel eksik)."
  exit 0
fi
echo "SONUC: ${blocking_missing} zorunlu parca eksik (${optional_missing} opsiyonel eksik)."
exit 1
