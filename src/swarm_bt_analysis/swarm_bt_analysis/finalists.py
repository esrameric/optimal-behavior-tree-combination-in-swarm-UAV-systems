"""
Finalist kombinasyon secimi - plan Bolum 4, Asama 2.

Plan: "En iyi 4-6 kombinasyonu sec. Secim kriteri SADECE performans degil,
N=3 -> N=5 arasi DAVRANIS DEGISIMI de dikkate alinir."

Bu modul o iki olcutu birlestirip siralar. Performans, en dusuk gorev
tamamlama suresi; davranis degisimi, Bolum 6'daki N-duyarlilik skoru.
"""

from __future__ import annotations

import argparse
import sys

import pandas as pd

from swarm_bt_analysis import ofat

#: Plan Bolum 4: "en iyi 4-6 kombinasyon".
DEFAULT_COUNT = 4

#: Performans ile olcek duyarliligi arasindaki agirlik. 0.5, planin
#: "sadece performans degil" vurgusuna karsilik gelen esit agirliktir.
DEFAULT_SENSITIVITY_WEIGHT = 0.5


def _normalize(series):
    """Seriyi [0,1] araligina tasir; tum degerler esitse 0 doner."""
    span = series.max() - series.min()
    if pd.isna(span) or span <= 0:
        return pd.Series(0.0, index=series.index)
    return (series - series.min()) / span


def rank(frame, metric='gorev_tamamlama_suresi',
         sensitivity_weight=DEFAULT_SENSITIVITY_WEIGHT):
    """
    Kombinasyonlari performans + olcek duyarliligi birlesik skoruyla siralar.

    Donen tabloda ``skor`` buyukten kucuge siralidir. Performans skoru, iki
    olcegin ORTALAMA metrik degerinden turer (dusuk daha iyi); duyarlilik
    skoru dogrudan N-duyarlilik ortalamasidir (yuksek olan daha ilgi cekici,
    cunku arastirma sorusu 2 tam olarak olcekle degisimi soruyor).
    """
    if metric not in frame.columns:
        raise ValueError(f'CSV {metric} sutununu icermiyor')

    performance = (
        frame.groupby('kombinasyon_id')[metric].mean()
        .rename('performans_metrigi').reset_index()
    )
    sensitivity = ofat.n_sensitivity(frame)[['kombinasyon_id', 'ortalama_duyarlilik']]
    merged = performance.merge(sensitivity, on='kombinasyon_id', how='left')
    merged['ortalama_duyarlilik'] = merged['ortalama_duyarlilik'].fillna(0.0)

    # Performans: dusuk sure iyi -> normalize edip tersle.
    merged['performans_skoru'] = 1.0 - _normalize(merged['performans_metrigi'])
    merged['duyarlilik_skoru'] = _normalize(merged['ortalama_duyarlilik'])
    merged['skor'] = (
        (1.0 - sensitivity_weight) * merged['performans_skoru']
        + sensitivity_weight * merged['duyarlilik_skoru']
    )
    return merged.sort_values('skor', ascending=False).reset_index(drop=True)


def select(frame, count=DEFAULT_COUNT, metric='gorev_tamamlama_suresi',
           sensitivity_weight=DEFAULT_SENSITIVITY_WEIGHT, always_include_baseline=True):
    """
    En iyi count kadar kombinasyonu secer.

    Baseline her zaman listeye alinir: Faz 2'de karsilastirma referansi olmadan
    diger finalistlerin sonuclari yorumlanamaz.
    """
    if count <= 0:
        raise ValueError('count pozitif olmali')

    ranked = rank(frame, metric, sensitivity_weight)
    chosen = list(ranked['kombinasyon_id'].head(count))

    if always_include_baseline:
        baseline = ofat.baseline_combination(frame)
        if baseline not in chosen:
            chosen = [baseline] + chosen[:count - 1]

    return ranked[ranked['kombinasyon_id'].isin(chosen)].reset_index(drop=True)


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(description='Faz 2 finalist kombinasyonlarini secer.')
    parser.add_argument('csv', help='ofat_sweep ciktisi (CSV)')
    parser.add_argument('-n', '--count', type=int, default=DEFAULT_COUNT,
                        help=f'finalist sayisi (plan: 4-6, varsayilan {DEFAULT_COUNT})')
    parser.add_argument('-w', '--sensitivity-weight', type=float,
                        default=DEFAULT_SENSITIVITY_WEIGHT,
                        help='olcek duyarliligi agirligi [0,1]')
    parser.add_argument('--ids-only', action='store_true',
                        help='yalnizca kombinasyon kimliklerini yaz')
    parser.add_argument('-o', '--output', help='CSV cikti dosyasi')
    args = parser.parse_args(argv)

    frame = ofat.load_sweep(args.csv)
    selected = select(frame, args.count, sensitivity_weight=args.sensitivity_weight)

    if args.ids_only:
        for combination in selected['kombinasyon_id']:
            print(combination)
        return 0
    if args.output:
        selected.to_csv(args.output, index=False)
        print(f'finalistler yazildi: {args.output}')
    else:
        selected.to_csv(sys.stdout, index=False)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
