"""
Faz 1 (kod-seviyesi) ile Faz 2 (Gazebo) sonuclarinin karsilastirmasi.

Plan Bolum 5/Faz 2 uc sey istiyor:
  1. Finalistleri N=3 ve N=5 ile Gazebo'da koştur (>= 5 tekrar)
  2. Karsilasma sikligini logla; N=5'te beklenen artisi dogrula
  3. Kod-seviyesi ile Gazebo sonuclarinin N-DUYARLILIGI ayni yonde mi kontrol et

Bu modul (2)'yi hesaplar; (3) bir sonraki adimda eklenir.
"""

from __future__ import annotations

import argparse
import sys

import pandas as pd

#: Faz etiketleri.
CODE_PHASE = 'kod'
GAZEBO_PHASE = 'gazebo'

#: Karsilastirilan metrikler.
COMPARED_METRICS = [
    'gorev_suresi',
    'tick',
    'karsilasma',
    'takas',
    'churn_orani',
    'atama_kararliligi',
    'carpisma',
]

_EPSILON = 1e-9


def load(path):
    """Faz 2 kampanya CSV'sini okur."""
    frame = pd.read_csv(path)
    required = {'kombinasyon_id', 'faz', 'N', 'tohum'}
    missing = required - set(frame.columns)
    if missing:
        raise ValueError(f'CSV eksik sutun iceriyor: {sorted(missing)}')
    return frame


def completion_report(frame):
    """Her (faz, kombinasyon, olcek) icin kapsama tamamlanma orani ve tekrar."""
    return (
        frame.groupby(['faz', 'kombinasyon_id', 'N'])
        .agg(tekrar=('tohum', 'count'), tamamlanma_orani=('kapsama_tamam', 'mean'))
        .reset_index()
    )


def encounter_scaling(frame):
    """
    Plan Bolum 5/Faz 2 (2): karsilasma sikligi N ile artiyor mu.

    Her faz ve kombinasyon icin N=3 ve N=5 ortalamalarini ve artis oranini verir.
    Bu, Bolum 9'da raporlanmasi istenen confound'un buyuklugudur.
    """
    means = (
        frame.groupby(['faz', 'kombinasyon_id', 'N'])['karsilasma']
        .mean().reset_index()
    )
    rows = []
    for (phase, combination), group in means.groupby(['faz', 'kombinasyon_id']):
        by_scale = dict(zip(group['N'], group['karsilasma']))
        if len(by_scale) < 2:
            continue
        low_key, high_key = sorted(by_scale)
        low, high = by_scale[low_key], by_scale[high_key]
        rows.append({
            'faz': phase,
            'kombinasyon_id': combination,
            f'karsilasma_N{low_key}': low,
            f'karsilasma_N{high_key}': high,
            'artis': high - low,
            'artis_katsayisi': high / low if low > _EPSILON else float('nan'),
            'artti_mi': high > low,
        })
    return pd.DataFrame(rows)


def _sign(value, tolerance=1e-9):
    if value > tolerance:
        return 1
    if value < -tolerance:
        return -1
    return 0


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(
        description='Faz 1 (kod) ile Faz 2 (Gazebo) sonuclarini karsilastirir.')
    parser.add_argument('csv', help='run_phase2.sh ciktisi (CSV)')
    parser.add_argument('-o', '--output', help='Markdown rapor dosyasi')
    args = parser.parse_args(argv)

    frame = load(args.csv)
    report = build_report(frame)

    if args.output:
        with open(args.output, 'w', encoding='utf-8') as handle:
            handle.write(report)
        print(f'rapor yazildi: {args.output}')
    else:
        sys.stdout.write(report)
    return 0


def build_report(frame):
    """Karsilastirma raporunun Markdown metnini uretir."""
    from swarm_bt_analysis.report_ofat import _format_table

    lines = ['# Faz 1 ↔ Faz 2 Karşılaştırması\n']
    completion = completion_report(frame)
    lines.append(f'- Kombinasyon sayısı: {frame["kombinasyon_id"].nunique()}')
    scales = ', '.join(f'N={int(n)}' for n in sorted(frame['N'].unique()))
    lines.append(f'- Ölçek değerleri: {scales}')
    lines.append(f'- Toplam koşu: {len(frame)}\n')

    lines.append('## 1. Tamamlanma ve Tekrar Denetimi\n')
    lines.append(_format_table(completion))

    lines.append('\n## 2. Karşılaşma Sıklığı Ölçekle Artıyor mu\n')
    lines.append(
        'Plan Bölüm 5/Faz 2: "N=5\'te beklenen artışı doğrula, bu confound\'u '
        'raporda açıkça belirt".\n')
    lines.append(_format_table(encounter_scaling(frame)))

    return '\n'.join(lines) + '\n'
