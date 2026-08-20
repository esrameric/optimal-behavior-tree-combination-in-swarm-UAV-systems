#!/usr/bin/env bash
# check_infra.sh'in kendisini sinar.
#
# Makineye bagimli olmamak icin "her sey kurulu mu" degil, RAPORUN BICIMI
# dogrulanir: beklenen basliklar var mi, her durum satiri [VAR]/[YOK]
# bicimine uyuyor mu, ozet satiri uretiliyor mu. Ek olarak, testlerin
# calisabilmesi icin zaten gerekli olan iki parcanin (ROS2, BT.CPP) VAR
# raporlandigi kontrol edilir -- bunlar yoksa bu test zaten calismazdi.
set -uo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
check_script="${script_dir}/../check_infra.sh"

failures=0
fail() {
  echo "BASARISIZ: $1" >&2
  failures=$((failures + 1))
}

if [[ ! -x "${check_script}" ]]; then
  fail "check_infra.sh bulunamadi ya da calistirilabilir degil: ${check_script}"
  exit 1
fi

output="$("${check_script}" 2>&1)"

for heading in "Faz 0 altyapi dogrulamasi" "Faz 1 (kod-seviyesi tarama) icin ZORUNLU" \
               "Faz 2 (Gazebo dogrulamasi) icin ZORUNLU" "OPSIYONEL" "SONUC:"; do
  grep -qF "${heading}" <<< "${output}" || fail "rapor '${heading}' basligini icermiyor"
done

# Her durum satiri "  [VAR] ad  ayrinti" bicimine uymali.
while IFS= read -r line; do
  [[ "${line}" =~ ^\ \ \[(VAR|YOK)\]\ [^[:space:]] ]] || fail "bozuk durum satiri: '${line}'"
done < <(grep -E '^\s*\[' <<< "${output}")

status_lines="$(grep -cE '^\s*\[' <<< "${output}")"
[[ "${status_lines}" -ge 10 ]] || fail "beklenenden az durum satiri: ${status_lines}"

# Bu test calisiyorsa ROS2 ve BT.CPP zaten kurulu olmali.
grep -qE '^\s*\[VAR\] ROS2' <<< "${output}" || fail "ROS2 VAR raporlanmadi"
grep -qE '^\s*\[VAR\] BehaviorTree\.CPP' <<< "${output}" || fail "BT.CPP VAR raporlanmadi"

# Fortress catismasi kontrolu plan Bolum 0'daki kritik uyari; hep raporlanmali.
grep -qF "Fortress catismasi" <<< "${output}" || fail "Fortress catismasi kontrolu raporlanmiyor"

if [[ ${failures} -eq 0 ]]; then
  echo "check_infra.sh: ${status_lines} durum satiri, rapor bicimi gecerli."
  exit 0
fi
echo "${failures} kontrol basarisiz." >&2
exit 1
