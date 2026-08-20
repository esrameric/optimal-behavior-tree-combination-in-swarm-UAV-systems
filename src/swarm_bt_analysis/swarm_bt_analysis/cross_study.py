"""
Iki calismanin olcek etkilerini karsilastirir - plan Bolum 5/Faz 4.

Plan: "Bu calismanin bulgularini yapilacaklar.md calismasinin heterojen
sonuclariyla karsilastir -- olcek etkisi rol-heterojen surude de benzer mi?"

Heterojen calismanin verisi bu repoda YOKTUR (ayri bir repo). Bu modul,
o veri geldiginde karsilastirmanin nasil yapilacagini kodda sabitler: iki
tarama CSV'si alir, ortak kombinasyonlarin N-duyarliliklarini eslestirir ve
olcek etkisinin ayni yonde olup olmadigini raporlar.

Boylece Faz 4, veri geldiginde tek komutluk bir ise indirgenir.
"""

from __future__ import annotations

import argparse
import sys

import pandas as pd

from swarm_bt_analysis import ofat

#: Calisma etiketleri.
HOMOGENEOUS = 'homojen'
HETEROGENEOUS = 'heterojen'

_EPSILON = 1e-9


def compare(homogeneous, heterogeneous, metrics=None):
    """
    Iki calismanin ortak kombinasyonlarini N-duyarlilik uzerinden karsilastirir.

    Donen tabloda her satir bir (kombinasyon, metrik) cifti; ``ayni_yon``,
    olcek etkisinin iki calismada ayni yonde olup olmadigini gosterir.
    """
    metrics = metrics or ofat.CORE_METRICS
    homo_deltas = ofat.scale_deltas(homogeneous, metrics)
    hetero_deltas = ofat.scale_deltas(heterogeneous, metrics)

    if homo_deltas.empty or hetero_deltas.empty:
        return pd.DataFrame(
            columns=['kombinasyon_id', 'metrik', 'delta_homojen',
                     'delta_heterojen', 'ayni_yon'])

    shared = (
        set(homo_deltas['kombinasyon_id']) & set(hetero_deltas['kombinasyon_id'])
    )
    if not shared:
        return pd.DataFrame(
            columns=['kombinasyon_id', 'metrik', 'delta_homojen',
                     'delta_heterojen', 'ayni_yon'])

    homo_indexed = homo_deltas.set_index('kombinasyon_id')
    hetero_indexed = hetero_deltas.set_index('kombinasyon_id')

    rows = []
    for combination in sorted(shared):
        for metric in metrics:
            column = f'{metric}_delta'
            if column not in homo_indexed.columns or column not in hetero_indexed.columns:
                continue
            homo_delta = homo_indexed.loc[combination, column]
            hetero_delta = hetero_indexed.loc[combination, column]
            rows.append({
                'kombinasyon_id': combination,
                'metrik': metric,
                'delta_homojen': homo_delta,
                'delta_heterojen': hetero_delta,
                'ayni_yon': _sign(homo_delta) == _sign(hetero_delta),
                'her_ikisi_de_etkisiz': _sign(homo_delta) == 0 and _sign(hetero_delta) == 0,
            })
    return pd.DataFrame(rows)


def agreement_summary(comparison):
    """Ortak kombinasyonlarda olcek etkisinin ne kadar ortustugunu ozetler."""
    if comparison.empty:
        return {'olculen': 0, 'ayni_yon': 0, 'oran': float('nan')}
    effective = comparison[~comparison['her_ikisi_de_etkisiz']]
    if effective.empty:
        return {'olculen': 0, 'ayni_yon': 0, 'oran': float('nan')}
    matched = int(effective['ayni_yon'].sum())
    return {
        'olculen': int(len(effective)),
        'ayni_yon': matched,
        'oran': matched / len(effective),
    }


def _sign(value, tolerance=_EPSILON):
    if value > tolerance:
        return 1
    if value < -tolerance:
        return -1
    return 0


def build_report(homogeneous, heterogeneous):
    """Iki calismanin karsilastirma raporunu uretir."""
    from swarm_bt_analysis.report_ofat import _format_table

    comparison = compare(homogeneous, heterogeneous)
    summary = agreement_summary(comparison)

    lines = ['# Faz 4 — Homojen ↔ Heterojen Ölçek Etkisi Karşılaştırması\n']
    lines.append(
        'Plan Bölüm 5/Faz 4: "ölçek etkisi rol-heterojen sürüde de benzer mi?"\n')

    if comparison.empty:
        lines.append(
            '**Karşılaştırılabilir ortak kombinasyon bulunamadı.** İki çalışmanın '
            'parametre uzayları örtüşmüyor ya da taramalardan biri tek ölçekli.\n')
        return '\n'.join(lines) + '\n'

    lines.append(
        f'- Ortak kombinasyon: {comparison["kombinasyon_id"].nunique()}\n'
        f'- Etkili ölçüm: {summary["olculen"]}\n'
        f'- Aynı yönde: {summary["ayni_yon"]} (%{100 * summary["oran"]:.0f})\n')

    mismatched = comparison[
        (~comparison['ayni_yon']) & (~comparison['her_ikisi_de_etkisiz'])
    ]
    lines.append('\n## Ölçek Etkisinin Ayrıştığı Ölçümler\n')
    if mismatched.empty:
        lines.append(
            'Ölçek etkisi tüm ortak ölçümlerde **aynı yönde**: rol heterojenliği, '
            'incelenen parametrelerin ölçekle davranışını değiştirmiyor.\n')
    else:
        lines.append(
            'Aşağıdaki ölçümlerde ölçek etkisi iki çalışmada **zıt yönde** — rol '
            'heterojenliği bu parametrelerin ölçek davranışını değiştiriyor.\n')
        lines.append(_format_table(
            mismatched.drop(columns=['her_ikisi_de_etkisiz'])))

    return '\n'.join(lines) + '\n'


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(
        description='Homojen ve heterojen calismalarin olcek etkilerini karsilastirir.')
    parser.add_argument('homojen_csv', help='bu calismanin tarama CSV\'si')
    parser.add_argument('heterojen_csv', help='heterojen calismanin tarama CSV\'si')
    parser.add_argument('-o', '--output', help='Markdown rapor dosyasi')
    args = parser.parse_args(argv)

    report = build_report(
        ofat.load_sweep(args.homojen_csv), ofat.load_sweep(args.heterojen_csv))

    if args.output:
        with open(args.output, 'w', encoding='utf-8') as handle:
            handle.write(report)
        print(f'rapor yazildi: {args.output}')
    else:
        sys.stdout.write(report)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
