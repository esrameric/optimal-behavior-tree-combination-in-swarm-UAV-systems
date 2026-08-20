#!/usr/bin/env bash
# BT.CPP apt paketi (ros-humble-behaviortree-cpp 4.9.1) icin prefix overlay uretir.
#
# Sorun: paket kutuphaneyi <prefix>/lib/<arch>/libbehaviortree_cpp.so altina
# kuruyor, ama ament_cmake_export_libraries shim'i onu <prefix>/lib icinde
# NO_DEFAULT_PATH ile ariyor -> mimariye ozgu alt dizine bakmiyor ->
# find_package(behaviortree_cpp) "couldn't be found" hatasiyla patliyor.
#
# Cozum: sudo gerektirmeyen, sembolik linklerden olusan kucuk bir prefix
# (.btcpp_overlay). CMAKE_PREFIX_PATH'e eklenince find_package sorunsuz calisir.
# Alternatif (sudo ile, kalici): 
#   sudo ln -sf <prefix>/lib/<arch>/libbehaviortree_cpp.so <prefix>/lib/
#
# Idempotent: tekrar calistirmak zararsiz.
set -euo pipefail

ROS_PREFIX="${ROS_PREFIX:-/opt/ros/humble}"
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OVERLAY="${REPO_ROOT}/.btcpp_overlay"
CMAKE_DIR="${ROS_PREFIX}/share/behaviortree_cpp/cmake"

if [[ ! -d "${CMAKE_DIR}" ]]; then
  echo "HATA: BT.CPP bulunamadi (${CMAKE_DIR}). Kurulum:" >&2
  echo "  sudo apt install ros-humble-behaviortree-cpp" >&2
  exit 1
fi

# Paket duzgunse (kutuphane dogrudan lib/ altinda) overlay'e gerek yok.
if [[ -e "${ROS_PREFIX}/lib/libbehaviortree_cpp.so" ]]; then
  echo "BT.CPP kutuphanesi ${ROS_PREFIX}/lib altinda -> overlay gerekmiyor."
  exit 0
fi

REAL_LIB="$(find "${ROS_PREFIX}/lib" -maxdepth 2 -name 'libbehaviortree_cpp.so*' -print -quit)"
if [[ -z "${REAL_LIB}" ]]; then
  echo "HATA: libbehaviortree_cpp.so ${ROS_PREFIX}/lib altinda bulunamadi." >&2
  exit 1
fi
REAL_LIB_DIR="$(dirname "${REAL_LIB}")"
ARCH="$(basename "${REAL_LIB_DIR}")"

rm -rf "${OVERLAY}"
mkdir -p "${OVERLAY}/lib" "${OVERLAY}/share/behaviortree_cpp"

# 1) ament_cmake_export_libraries'in baktigi yol: <overlay>/lib/libbehaviortree_cpp.so
ln -sf "${REAL_LIB}" "${OVERLAY}/lib/libbehaviortree_cpp.so"
# 2) TargetsExport'un IMPORTED_LOCATION'i: <overlay>/lib/<arch>/libbehaviortree_cpp.so
ln -sf "${REAL_LIB_DIR}" "${OVERLAY}/lib/${ARCH}"
# 3) export_include_directories'in baktigi yol: <overlay>/include
ln -sf "${ROS_PREFIX}/include" "${OVERLAY}/include"
# 4) cmake config dosyalari (kopya; _IMPORT_PREFIX bunlarin konumundan turetilir)
cp -r "${CMAKE_DIR}" "${OVERLAY}/share/behaviortree_cpp/"
[[ -f "${ROS_PREFIX}/share/behaviortree_cpp/package.xml" ]] && \
  cp "${ROS_PREFIX}/share/behaviortree_cpp/package.xml" "${OVERLAY}/share/behaviortree_cpp/"

echo "BT.CPP overlay hazir: ${OVERLAY}"
echo "  kutuphane: ${REAL_LIB}"
echo "  mimari   : ${ARCH}"
