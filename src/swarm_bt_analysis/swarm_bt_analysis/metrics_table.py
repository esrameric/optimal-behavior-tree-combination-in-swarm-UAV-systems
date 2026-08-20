"""
Metrik tablosu uretimi - plan Bolum 5/Faz 1 ve Bolum 6.

Tarama ciktisindan, her kombinasyon icin N=3 ve N=5 degerlerini yan yana koyan
ve aralarindaki DELTA sutununu dolduran tabloyu uretir. Plan Bolum 5/Faz 1
bu delta sutununu acikca istiyor; Bolum 6'daki N-duyarlilik skoru da ayni
tablodan turer.
"""

from __future__ import annotations

import argparse
import sys

from swarm_bt_analysis import ofat


def build_table(frame, metrics=None):
    """Kombinasyon basina N=3 / N=5 / delta / duyarlilik sutunlarini birlestirir."""
    metrics = metrics or ofat.CORE_METRICS
    deltas = ofat.scale_deltas(frame, metrics)
    sensitivity = ofat.n_sensitivity(frame, metrics)

    if deltas.empty:
        return deltas

    sensitivity = sensitivity.rename(
        columns={m: f'{m}_duyarlilik' for m in metrics if m in sensitivity.columns})
    merged = deltas.merge(sensitivity, on='kombinasyon_id', how='left')

    # Sutun sirasi: kombinasyon, sonra her metrik icin N3 / N5 / delta / duyarlilik
    ordered = ['kombinasyon_id']
    for metric in metrics:
        for suffix in ('_N3', '_N5', '_delta', '_duyarlilik'):
            column = f'{metric}{suffix}'
            if column in merged.columns:
                ordered.append(column)
    if 'ortalama_duyarlilik' in merged.columns:
        ordered.append('ortalama_duyarlilik')

    return merged[ordered].sort_values('ortalama_duyarlilik', ascending=False)


def build_markdown(frame, metrics=None):
    """Metrik tablosunun okunabilir Markdown ozeti."""
    metrics = metrics or ofat.CORE_METRICS
    table = build_table(frame, metrics)
    lines = ['# Metrik Tablosu — N=3 → N=5 Deltalari\n']
    lines.append(f'- Kombinasyon sayisi: {len(table)}')
    lines.append(f'- Baseline: `{ofat.baseline_combination(frame)}`\n')

    for metric in metrics:
        columns = [f'{metric}_N3', f'{metric}_N5', f'{metric}_delta']
        if not all(column in table.columns for column in columns):
            continue
        lines.append(f'\n## {metric}\n')
        view = table[['kombinasyon_id', *columns]].copy()
        view = view.sort_values(f'{metric}_delta')
        header = ['kombinasyon_id', 'N=3', 'N=5', 'delta']
        rows = [
            [str(record[0]), f'{record[1]:.3f}', f'{record[2]:.3f}', f'{record[3]:+.3f}']
            for record in view.itertuples(index=False)
        ]
        widths = [max(len(header[i]), *(len(row[i]) for row in rows)) for i in range(4)]
        lines.append('| ' + ' | '.join(h.ljust(widths[i]) for i, h in enumerate(header)) + ' |')
        lines.append('| ' + ' | '.join('-' * w for w in widths) + ' |')
        for row in rows:
            lines.append('| ' + ' | '.join(v.ljust(widths[i]) for i, v in enumerate(row)) + ' |')

    return '\n'.join(lines) + '\n'


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(
        description='Tarama ciktisindan N=3 -> N=5 delta tablosu uretir.')
    parser.add_argument('csv', help='ofat_sweep ciktisi (CSV)')
    parser.add_argument('-o', '--output', help='CSV cikti dosyasi')
    parser.add_argument('-m', '--markdown', help='Markdown ozet dosyasi')
    parser.add_argument(
        '--skip-repetition-check', action='store_true',
        help='tekrar sayisi denetimini atla (yalnizca on inceleme icin)')
    args = parser.parse_args(argv)

    frame = ofat.load_sweep(args.csv)
    if not args.skip_repetition_check:
        ofat.assert_enough_repetitions(frame)

    table = build_table(frame)

    if args.output:
        table.to_csv(args.output, index=False)
        print(f'metrik tablosu yazildi: {args.output}')
    if args.markdown:
        with open(args.markdown, 'w', encoding='utf-8') as handle:
            handle.write(build_markdown(frame))
        print(f'Markdown ozet yazildi: {args.markdown}')
    if not args.output and not args.markdown:
        table.to_csv(sys.stdout, index=False)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
