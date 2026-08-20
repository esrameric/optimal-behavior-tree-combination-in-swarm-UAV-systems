"""

OFAT tarama ciktisinin olcek-duyarlilik analizi (plan Bolum 4 ve Bolum 6).

Plan Bolum 4: "her parametre degisiminin IKI N DEGERINDE DE ayni yonde mi etki
ettigini karsilastir (ayni yonde degilse bu, parametrenin N'e duyarli oldugu
anlamina gelir -- bulgunun kendisi)".

Bu modul o karsilastirmayi yapar ve Bolum 6'daki N-duyarlilik skorunu hesaplar.

"""

from __future__ import annotations

import pandas as pd

#: Bolum 6 temel metrikleri + bu calismaya ozgu metrikler.
CORE_METRICS = [
    'gorev_tamamlama_suresi',
    'atama_kararliligi',
    'churn_orani',
    'kapsama_dengesizligi',
    'karsilasma_sikligi',
    'iletisim_yuku',
    'tick_maliyeti',
    'carpisma_sayisi',
]

#: Kucuk paydalarda oransal degisimin patlamasini onleyen esik.
_EPSILON = 1e-9

#: Plan Bolum 5/Faz 1: her kombinasyon x olcek icin en az bu kadar tekrar.
#: Koşular rastgele kalkis konumlarindan ve hiz sapmasindan etkilendigi icin
#: tek bir tohum yaniltici; ortalama alinabilmesi buna bagli.
MIN_REPETITIONS = 10


def load_sweep(path):
    """OFAT tarama CSV'sini okur ve tip donuşumlerini yapar."""
    frame = pd.read_csv(path)
    required = {'deney_id', 'kombinasyon_id', 'N'}
    missing = required - set(frame.columns)
    if missing:
        raise ValueError(f'CSV eksik sutun iceriyor: {sorted(missing)}')
    return frame


def repetition_report(frame, minimum=MIN_REPETITIONS):
    """
    Her (kombinasyon, olcek) ciftinin tekrar sayisini denetler.

    Donen tabloda ``yeterli`` sutunu False olan satirlar plan Bolum 5/Faz 1'in
    ">= 10 tekrar" sartini saglamiyor demektir.
    """
    if 'tekrar' not in frame.columns:
        raise ValueError("CSV 'tekrar' sutunu icermiyor: tekrar sayisi denetlenemez")

    rows = []
    for (combination, n_agents), group in frame.groupby(['kombinasyon_id', 'N']):
        repetitions = int(group['tekrar'].min())
        rows.append({
            'kombinasyon_id': combination,
            'N': int(n_agents),
            'tekrar': repetitions,
            'yeterli': repetitions >= minimum,
        })
    return pd.DataFrame(rows).sort_values(['yeterli', 'kombinasyon_id', 'N'])


def assert_enough_repetitions(frame, minimum=MIN_REPETITIONS):
    """
    Yetersiz tekrar iceren satir varsa ValueError atar.

    Analiz zincirinin basinda cagrilir: yetersiz tekrarla uretilmis bir
    tabloya dayanan sonuclar yayinlanmamali.
    """
    report = repetition_report(frame, minimum)
    insufficient = report[~report['yeterli']]
    if not insufficient.empty:
        detail = ', '.join(
            f"{row['kombinasyon_id']}@N={row['N']}({row['tekrar']})"
            for _, row in insufficient.iterrows()
        )
        raise ValueError(
            f'{len(insufficient)} satir {minimum} tekrarin altinda: {detail}'
        )
    return report


def baseline_combination(frame):
    """Taramadaki baseline kombinasyonunun kimligi (CSV'nin ilk satiri)."""
    return frame.iloc[0]['kombinasyon_id']


def effect_table(frame, metrics=None):
    """
    Her kombinasyonun baseline'a gore etkisini, her N degeri icin verir.

    Donen tabloda her satir bir (kombinasyon, N) cifti; ``delta`` sutunlari
    baseline'dan farki, ``delta_orani`` ise bu farkin baseline'a oranini tasir.

    """
    metrics = metrics or CORE_METRICS
    baseline_id = baseline_combination(frame)
    rows = []

    for n_agents, group in frame.groupby('N'):
        base = group[group['kombinasyon_id'] == baseline_id]
        if base.empty:
            raise ValueError(f'N={n_agents} icin baseline satiri yok')
        base = base.iloc[0]

        for _, row in group.iterrows():
            if row['kombinasyon_id'] == baseline_id:
                continue
            record = {
                'kombinasyon_id': row['kombinasyon_id'],
                'N': n_agents,
                'degisen_eksen': changed_axis(base, row),
            }
            for metric in metrics:
                if metric not in frame.columns:
                    continue
                delta = row[metric] - base[metric]
                record[f'{metric}_delta'] = delta
                denominator = abs(base[metric])
                record[f'{metric}_delta_orani'] = (
                    delta / denominator if denominator > _EPSILON else float('nan')
                )
            rows.append(record)

    return pd.DataFrame(rows)


def changed_axis(baseline_row, row):
    """Bu kombinasyonun baseline'dan hangi eksende ayrildigi (orn. 'P4')."""
    for axis in ('P2', 'P3', 'P4', 'P5', 'P6'):
        if str(baseline_row[axis]) != str(row[axis]):
            return axis
    return 'baseline'


def direction_agreement(frame, metrics=None, tolerance=1e-9):
    """
    Bir parametre degisiminin iki N degerinde ayni yonde etki edip etmedigi.

    ``ayni_yon`` False olan satirlar, plan Bolum 4'un aradigi bulgudur:
    o parametre N'e duyarlidir.

    """
    metrics = metrics or CORE_METRICS
    effects = effect_table(frame, metrics)
    rows = []

    for (combination, axis), group in effects.groupby(['kombinasyon_id', 'degisen_eksen']):
        by_scale = {int(row['N']): row for _, row in group.iterrows()}
        if len(by_scale) < 2:
            continue
        scales = sorted(by_scale)
        low, high = by_scale[scales[0]], by_scale[scales[-1]]

        for metric in metrics:
            column = f'{metric}_delta'
            if column not in group.columns:
                continue
            low_delta = low[column]
            high_delta = high[column]
            low_sign = _sign(low_delta, tolerance)
            high_sign = _sign(high_delta, tolerance)
            rows.append({
                'kombinasyon_id': combination,
                'degisen_eksen': axis,
                'metrik': metric,
                f'delta_N{scales[0]}': low_delta,
                f'delta_N{scales[-1]}': high_delta,
                'ayni_yon': low_sign == high_sign,
                # Gercek yon CATISMASI: iki tarafta da etki var ve zit yonde.
                # "Bir tarafta etki yok" durumu catisma degil, olcek esigidir.
                'zit_yon': low_sign * high_sign < 0,
                'tek_tarafli_etki': (low_sign == 0) != (high_sign == 0),
                'her_ikisi_de_etkisiz': low_sign == 0 and high_sign == 0,
            })

    return pd.DataFrame(rows)


def n_sensitivity(frame, metrics=None):
    """
    Bolum 6 - N-duyarlilik skoru: |N=5 degeri - N=3 degeri| / N=3 degeri.

    Her kombinasyon ve her metrik icin bir satir doner.

    """
    metrics = metrics or CORE_METRICS
    rows = []

    for combination, group in frame.groupby('kombinasyon_id'):
        by_scale = {int(row['N']): row for _, row in group.iterrows()}
        if len(by_scale) < 2:
            continue
        scales = sorted(by_scale)
        low, high = by_scale[scales[0]], by_scale[scales[-1]]

        record = {'kombinasyon_id': combination}
        for metric in metrics:
            if metric not in frame.columns:
                continue
            denominator = abs(low[metric])
            record[metric] = (
                abs(high[metric] - low[metric]) / denominator
                if denominator > _EPSILON else float('nan')
            )
        record['ortalama_duyarlilik'] = pd.Series(
            {k: v for k, v in record.items() if k != 'kombinasyon_id'}
        ).mean(skipna=True)
        rows.append(record)

    if not rows:
        # Tek olcekli tarama: karsilastirilacak cift yok.
        columns = ['kombinasyon_id', *[m for m in metrics if m in frame.columns],
                   'ortalama_duyarlilik']
        return pd.DataFrame(columns=columns)

    return pd.DataFrame(rows).sort_values('ortalama_duyarlilik', ascending=False)


def scale_deltas(frame, metrics=None):
    """Her kombinasyon icin N=3 -> N=5 ham metrik degisimi (plan Bolum 5/Faz 1)."""
    metrics = metrics or CORE_METRICS
    rows = []

    for combination, group in frame.groupby('kombinasyon_id'):
        by_scale = {int(row['N']): row for _, row in group.iterrows()}
        if len(by_scale) < 2:
            continue
        scales = sorted(by_scale)
        low, high = by_scale[scales[0]], by_scale[scales[-1]]
        record = {'kombinasyon_id': combination}
        for metric in metrics:
            if metric not in frame.columns:
                continue
            record[f'{metric}_N{scales[0]}'] = low[metric]
            record[f'{metric}_N{scales[-1]}'] = high[metric]
            record[f'{metric}_delta'] = high[metric] - low[metric]
        rows.append(record)

    return pd.DataFrame(rows)


def _sign(value, tolerance):
    """Toleransli isaret: gurultu duzeyindeki farklar 0 sayilir."""
    if value > tolerance:
        return 1
    if value < -tolerance:
        return -1
    return 0
