"""
Parametre eksenlerinin olcek duyarliligi - plan Bolum 5/Faz 3.

Plan: "Hangi parametrelerin N-duyarliligi en yuksek/duşuk oldugunu belirle
(orn. tam merkezi mimari N arttikca mi bozuluyor, tam dagitik mi daha dayanikli
cikiyor?)"

ofat.n_sensitivity() KOMBINASYON basina skor verir. Bu modul o skoru
PARAMETRE EKSENINE ve EKSEN SECENEGINE tasir: her OFAT varyanti baseline'dan
tek bir eksende ayrildigi icin, o varyantin duyarliligi o eksene atfedilebilir.
"""

from __future__ import annotations

import argparse
import sys

import pandas as pd

from swarm_bt_analysis import ofat

#: Parametre eksenleri.
AXES = ['P2', 'P3', 'P4', 'P5', 'P6']

#: Eksen seceneklerinin okunabilir adlari (plan Bolum 3 tablosu).
OPTION_NAMES = {
    ('P2', 'a'): 'tam merkezi',
    ('P2', 'b'): 'hiyerarsik hibrit',
    ('P2', 'c'): 'tam dagitik',
    ('P3', 'a'): 'statik esit bolme',
    ('P3', 'b'): 'Contract Net',
    ('P3', 'c'): 'CBBA',
    ('P4', 'a'): 'tek merkezi BT',
    ('P4', 'b'): 'ozdes dagitik BT',
    ('P4', 'c'): 'olay-gudumlu BT',
    ('P6', 'a'): 'periyodik yoklama',
    ('P6', 'b'): 'her tick kontrol',
    ('P6', 'c'): 'saf olay-tetiklemeli',
}


#: P5 birlestirilebilir oldugu icin (orn. "abcd") harf harf cozulur.
P5_MECHANISMS = {
    'a': 'dogrudan mesaj',
    'b': 'stigmerji',
    'c': 'intent yayini',
    'd': 'kulak misafiri',
}


def option_name(axis, option):
    """Eksen secenegi icin okunabilir ad; bilinmiyorsa harfin kendisi."""
    option = str(option)
    if axis == 'P5':
        if option == 'none':
            return 'iletisim yok'
        parts = [P5_MECHANISMS[letter] for letter in option if letter in P5_MECHANISMS]
        return ' + '.join(parts) if parts else f'P5{option}'
    return OPTION_NAMES.get((axis, option), f'{axis}{option}')


def per_variant(frame):
    """
    Her OFAT varyantini degistirdigi eksenle ve duyarlilik skoruyla eslestirir.

    Baseline satiri disarida birakilir: hicbir ekseni degistirmiyor.
    """
    baseline_id = ofat.baseline_combination(frame)
    baseline_row = frame[frame['kombinasyon_id'] == baseline_id].iloc[0]
    sensitivity = ofat.n_sensitivity(frame).set_index('kombinasyon_id')

    rows = []
    for combination, group in frame.groupby('kombinasyon_id'):
        if combination == baseline_id or combination not in sensitivity.index:
            continue
        row = group.iloc[0]
        axis = ofat.changed_axis(baseline_row, row)
        if axis == 'baseline':
            continue
        rows.append({
            'kombinasyon_id': combination,
            'eksen': axis,
            'secenek': str(row[axis]),
            'secenek_adi': option_name(axis, row[axis]),
            'duyarlilik': sensitivity.loc[combination, 'ortalama_duyarlilik'],
        })
    return pd.DataFrame(rows)


def by_axis(frame):
    """
    Eksen basina duyarlilik ozeti: ortalama, en yuksek ve varyant sayisi.

    Sonuc ortalama duyarliliga gore azalan siralidir -- planin sordugu
    "hangi parametrenin N-duyarliligi en yuksek" sorusunun dogrudan cevabi.
    """
    variants = per_variant(frame)
    if variants.empty:
        return pd.DataFrame(columns=['eksen', 'varyant', 'ortalama', 'en_yuksek', 'en_duşuk'])

    summary = (
        variants.groupby('eksen')['duyarlilik']
        .agg(varyant='count', ortalama='mean', en_yuksek='max', en_duşuk='min')
        .reset_index()
        .sort_values('ortalama', ascending=False)
    )
    return summary


def by_option(frame):
    """Eksen SECENEGI basina duyarlilik; azalan sirali."""
    variants = per_variant(frame)
    if variants.empty:
        return variants
    return variants.sort_values('duyarlilik', ascending=False).reset_index(drop=True)


def architecture_comparison(frame, metric='gorev_tamamlama_suresi'):
    """
    Plan Bolum 5/Faz 3'un acikca sordugu karsilastirma.

    "Tam merkezi mimari N arttikca mi bozuluyor, tam dagitik mi daha dayanikli
    cikiyor?" -- P2 seceneklerinin metrigi iki olcekte nasil degistigini verir.
    """
    if metric not in frame.columns:
        raise ValueError(f'CSV {metric} sutununu icermiyor')

    baseline_id = ofat.baseline_combination(frame)
    baseline_row = frame[frame['kombinasyon_id'] == baseline_id].iloc[0]
    scales = sorted(int(n) for n in frame['N'].unique())
    low, high = scales[0], scales[-1]

    # Baseline P2c olduguna gore, P2 ekseninin tum secenekleri: baseline + varyantlar.
    candidates = {baseline_id}
    for combination, group in frame.groupby('kombinasyon_id'):
        if ofat.changed_axis(baseline_row, group.iloc[0]) == 'P2':
            candidates.add(combination)

    rows = []
    for combination in sorted(candidates):
        group = frame[frame['kombinasyon_id'] == combination]
        by_scale = {int(row['N']): row for _, row in group.iterrows()}
        if low not in by_scale or high not in by_scale:
            continue
        option = str(by_scale[low]['P2'])
        low_value = by_scale[low][metric]
        high_value = by_scale[high][metric]
        rows.append({
            'P2': option,
            'mimari': option_name('P2', option),
            f'{metric}_N{low}': low_value,
            f'{metric}_N{high}': high_value,
            'delta': high_value - low_value,
        })

    table = pd.DataFrame(rows)
    if table.empty:
        return table

    # Baseline'a gore goreli konum: hangi mimari hangi olcekte daha iyi.
    baseline_low = table[table['P2'] == str(baseline_row['P2'])][f'{metric}_N{low}'].iloc[0]
    baseline_high = table[table['P2'] == str(baseline_row['P2'])][f'{metric}_N{high}'].iloc[0]
    table[f'baseline_farki_N{low}'] = table[f'{metric}_N{low}'] - baseline_low
    table[f'baseline_farki_N{high}'] = table[f'{metric}_N{high}'] - baseline_high
    return table.sort_values('P2')


def build_report(frame):
    """Parametre duyarliligi raporunun Markdown metnini uretir."""
    from swarm_bt_analysis.report_ofat import _format_table

    scales = sorted(int(n) for n in frame['N'].unique())
    low, high = scales[0], scales[-1]
    axis_summary = by_axis(frame)

    lines = ['# Parametre Eksenlerinin Ölçek Duyarlılığı\n']
    lines.append(
        f'Plan Bölüm 5/Faz 3: "Hangi parametrelerin N-duyarlılığı en '
        f'yüksek/düşük olduğunu belirle." Ölçek: N={low} → N={high}.\n')
    lines.append(
        'Her OFAT varyantı baseline\'dan **tek bir eksende** ayrıldığı için, o '
        'varyantın N-duyarlılık skoru o eksene atfedilebilir.\n')

    lines.append('## 1. Eksen Sıralaması\n')
    if not axis_summary.empty:
        highest = axis_summary.iloc[0]
        lowest = axis_summary.iloc[-1]
        lines.append(
            f'- **En duyarlı eksen: {highest["eksen"]}** '
            f'(ortalama {highest["ortalama"]:.3f})\n'
            f'- **En az duyarlı eksen: {lowest["eksen"]}** '
            f'(ortalama {lowest["ortalama"]:.3f})\n')
    lines.append(_format_table(axis_summary))

    lines.append('\n## 2. Seçenek Bazında\n')
    options = by_option(frame)
    columns = ['eksen', 'secenek', 'secenek_adi', 'duyarlilik', 'kombinasyon_id']
    lines.append(_format_table(options[columns] if not options.empty else options))

    lines.append('\n## 3. Merkezi mi Dağıtık mı — planın açık sorusu\n')
    lines.append(
        '"Tam merkezi mimari N arttıkça mı bozuluyor, tam dağıtık mı daha '
        'dayanıklı çıkıyor?"\n')
    comparison = architecture_comparison(frame)
    lines.append(_format_table(comparison))

    if not comparison.empty:
        central = comparison[comparison['P2'] == 'a']
        distributed = comparison[comparison['P2'] == 'c']
        if not central.empty and not distributed.empty:
            central_gap_low = central[f'baseline_farki_N{low}'].iloc[0]
            central_gap_high = central[f'baseline_farki_N{high}'].iloc[0]
            if central_gap_low < 0 <= central_gap_high:
                lines.append(
                    f'\n**Cevap:** tam merkezi mimari N={low}\'te dağıtıktan '
                    f'**iyi** ({central_gap_low:+.2f} s), N={high}\'te '
                    f'**kötü** ({central_gap_high:+.2f} s). Merkezi koordinasyon '
                    'ölçekle bozuluyor; tam dağıtık daha dayanıklı.\n')
            else:
                lines.append(
                    f'\n**Cevap:** merkezi ile dağıtık arasındaki fark '
                    f'N={low}\'te {central_gap_low:+.2f}, N={high}\'te '
                    f'{central_gap_high:+.2f} s — yön değişimi gözlenmedi.\n')

    return '\n'.join(lines) + '\n'


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(
        description='Parametre eksenlerinin olcek duyarliligini raporlar.')
    parser.add_argument('csv', help='ofat_sweep ciktisi (CSV)')
    parser.add_argument('-o', '--output', help='Markdown rapor dosyasi')
    args = parser.parse_args(argv)

    frame = ofat.load_sweep(args.csv)
    report = build_report(frame)

    if args.output:
        with open(args.output, 'w', encoding='utf-8') as handle:
            handle.write(report)
        print(f'rapor yazildi: {args.output}')
    else:
        sys.stdout.write(report)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
