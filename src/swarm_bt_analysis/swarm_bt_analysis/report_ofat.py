"""
OFAT tarama ciktisindan Markdown rapor ureten komut satiri araci.

Kullanim:
    ros2 run swarm_bt_analysis ofat_report <tarama.csv> [-o rapor.md]
"""

from __future__ import annotations

import argparse
import sys

import pandas as pd

from swarm_bt_analysis import ofat


def _format_table(frame, float_format='{:.3f}'):
    """
    Pandas tablosunu Markdown tablosuna cevirir.

    pandas.to_markdown "tabulate" paketini gerektiriyor; tek bir tablo bicimi
    icin harici bagimlilik eklemek yerine bicimlendirme burada yapilir.
    """
    if frame.empty:
        return '_(veri yok)_\n'

    def cell(value):
        if isinstance(value, float):
            return '-' if pd.isna(value) else float_format.format(value)
        return str(value)

    headers = [str(column) for column in frame.columns]
    rows = [[cell(value) for value in record] for record in frame.itertuples(index=False)]
    widths = []
    for index, header in enumerate(headers):
        width = len(header)
        for row in rows:
            width = max(width, len(row[index]))
        widths.append(width)

    def line(values):
        return '| ' + ' | '.join(v.ljust(widths[i]) for i, v in enumerate(values)) + ' |'

    out = [line(headers), '| ' + ' | '.join('-' * w for w in widths) + ' |']
    out.extend(line(row) for row in rows)
    return '\n'.join(out) + '\n'


def build_report(frame):
    """Tarama tablosundan Markdown rapor metni uretir."""
    baseline = ofat.baseline_combination(frame)
    scales = sorted(frame['N'].unique())
    lines = []

    lines.append('# OFAT Taramasi — Olcek Duyarliligi\n')
    lines.append(f'- Baseline kombinasyon: `{baseline}`')
    lines.append(f'- Olcek degerleri: {", ".join(f"N={n}" for n in scales)}')
    lines.append(f'- Kombinasyon sayisi: {frame["kombinasyon_id"].nunique()}')
    lines.append(f'- Toplam satir: {len(frame)}')
    if 'tekrar' in frame.columns:
        lines.append(f'- Kombinasyon x olcek basina tekrar: {int(frame["tekrar"].iloc[0])}')
    lines.append('')

    # --- 0. Tekrar denetimi (plan Bolum 5/Faz 1) ---
    repetitions = ofat.repetition_report(frame)
    insufficient = repetitions[~repetitions['yeterli']]
    lines.append('## 0. Tekrar Denetimi\n')
    if insufficient.empty:
        lines.append(
            f'Tum {len(repetitions)} (kombinasyon, olcek) cifti en az '
            f'{ofat.MIN_REPETITIONS} tekrar iceriyor.\n')
    else:
        lines.append(
            f'**UYARI:** {len(insufficient)} cift {ofat.MIN_REPETITIONS} tekrarin '
            'altinda; asagidaki sonuclar guvenilir degil.\n')
        lines.append(_format_table(insufficient, '{:.0f}'))

    # --- 1. Yon catismalari: planin aradigi asil bulgu ---
    agreement = ofat.direction_agreement(frame)
    columns = ['kombinasyon_id', 'degisen_eksen', 'metrik',
               *[c for c in agreement.columns if c.startswith('delta_N')]]
    conflicts = agreement[agreement['zit_yon']][columns]

    lines.append('## 1. Olcege Duyarli Parametreler (yon CATISMASI)\n')
    lines.append(
        'Plan Bolum 4: bir parametre degisimi iki N degerinde ZIT yonde etki '
        'ediyorsa, o parametre olcege duyarlidir — bulgunun kendisi budur.\n\n'
        'Burada yalnizca IKI TARAFTA DA etki olan ve yonleri zit olan olcumler '
        'listelenir. "Bir olcekte etki var, digerinde yok" durumu ayri '
        'baslikta verilir: o bir catisma degil, etkinin ortaya ciktigi olcek '
        'esigidir.\n'
    )
    if conflicts.empty:
        lines.append('_Hicbir parametrede yon catismasi bulunmadi._\n')
    else:
        lines.append(_format_table(conflicts))

    # --- 2. Eksen bazinda ozet ---
    lines.append('\n## 2. Eksen Bazinda Ozet\n')
    summary = (
        agreement.groupby('degisen_eksen')
        .agg(
            olculen=('metrik', 'count'),
            yon_catismasi=('zit_yon', 'sum'),
            tek_tarafli=('tek_tarafli_etki', 'sum'),
            etkisiz=('her_ikisi_de_etkisiz', 'sum'),
        )
        .reset_index()
        .sort_values('yon_catismasi', ascending=False)
    )
    lines.append(_format_table(summary, '{:.0f}'))

    # --- 2b. Olcek esigi: bir olcekte etki, digerinde yok ---
    lines.append('\n### 2b. Olcek Esigi Gosteren Olcumler\n')
    lines.append(
        'Etki yalnizca bir olcekte ortaya cikiyor; parametre o esigin '
        'altinda/ustunde davranis degistiriyor.\n'
    )
    threshold = agreement[agreement['tek_tarafli_etki']][columns]
    lines.append(_format_table(threshold))

    # --- 3. N-duyarlilik skoru (Bolum 6) ---
    lines.append('\n## 3. N-Duyarlilik Skoru (Bolum 6)\n')
    lines.append('`|N=5 degeri − N=3 degeri| / N=3 degeri`, kombinasyon basina.\n')
    lines.append(
        '`-` isareti, N=3 degerinin SIFIR oldugunu ve oransal degisimin tanimsiz '
        'kaldigini gosterir. Bu kendi basina bir bulgudur: metrik N=3\'te hic '
        'hareket etmiyor, N=5\'te ediyor (orn. churn orani -- takas mekanizmasi '
        'N=3\'te hic tetiklenmiyor).\n')
    lines.append(_format_table(ofat.n_sensitivity(frame)))

    # --- 4. Etkisiz eksenler ---
    lines.append('\n## 4. Hicbir Olcekte Etki Gostermeyen Eksenler\n')
    inert = (
        agreement.groupby('degisen_eksen')
        .agg(olculen=('metrik', 'count'), etkisiz=('her_ikisi_de_etkisiz', 'sum'))
        .reset_index()
    )
    if not inert.empty:
        inert['etkisiz_orani'] = inert['etkisiz'] / inert['olculen']
        inert = inert.sort_values('etkisiz_orani', ascending=False)
    lines.append(_format_table(inert))

    return '\n'.join(lines)


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(description='OFAT tarama raporu uretir.')
    parser.add_argument('csv', help='ofat_sweep ciktisi (CSV)')
    parser.add_argument('-o', '--output', help='rapor dosyasi; verilmezse stdout')
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
