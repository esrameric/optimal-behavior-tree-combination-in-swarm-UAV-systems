"""
"Manipulasyona aciklik" hipotez testi - plan Bolum 5/Faz 3.

Arastirma sorusu 2: "BT'nin karar mekanizmasi (ozellikle dinamik
yeniden-atama) ajan sayisi arttikca daha kararsiz/oynak mi (manipulasyona
acik) yoksa daha kararli mi (dayanikli) hale geliyor?"

Plan Bolum 6 bu soruyu su metrikle operasyonellestiriyor:
  atama kararliligi = bir drone'un atanmis alaninin koşu boyunca kac kez
                      degistigi, ajan basina ortalama

DIKKAT - METRIGIN YONU: bu sayac YUKSELDIKCE sistem DAHA OYNAK olur. Yani
"atama kararliligi" adi, olculen seyin TERSINI cagristiriyor. Bu modul
degerleri yorumlarken bunu acikca ele alir: sayac duşuyorsa sistem
DAHA KARARLI (dayanikli), yukseliyorsa DAHA OYNAK (manipulasyona acik).
"""

from __future__ import annotations

import argparse
import sys

import pandas as pd

from swarm_bt_analysis import ofat

#: Hipotezin operasyonellestirildigi metrikler.
STABILITY_METRIC = 'atama_kararliligi'
CHURN_METRIC = 'churn_orani'

#: Sonuc etiketleri.
MORE_STABLE = 'daha_kararli'
MORE_VOLATILE = 'daha_oynak'
UNCHANGED = 'degismedi'

_EPSILON = 1e-9


def classify(delta, tolerance=_EPSILON):
    """
    Atama degisikligi sayacindaki degisimi yorumlar.

    Sayac duşuyorsa (delta < 0) sistem DAHA KARARLI; yukseliyorsa DAHA OYNAK.
    """
    if delta < -tolerance:
        return MORE_STABLE
    if delta > tolerance:
        return MORE_VOLATILE
    return UNCHANGED


def evaluate(frame, metric=STABILITY_METRIC):
    """
    Her kombinasyon icin N=3 -> N=5 kararlilik degisimini siniflar.

    Donen tabloda ``sonuc`` sutunu MORE_STABLE / MORE_VOLATILE / UNCHANGED
    degerlerinden birini alir.
    """
    if metric not in frame.columns:
        raise ValueError(f'CSV {metric} sutununu icermiyor')

    deltas = ofat.scale_deltas(frame, [metric])
    if deltas.empty:
        return pd.DataFrame(
            columns=['kombinasyon_id', f'{metric}_N3', f'{metric}_N5',
                     f'{metric}_delta', 'sonuc'])

    low = sorted(int(n) for n in frame['N'].unique())[0]
    deltas = deltas.copy()
    deltas['sonuc'] = deltas[f'{metric}_delta'].map(classify)
    deltas['bagil_degisim'] = deltas.apply(
        lambda row: (
            row[f'{metric}_delta'] / abs(row[f'{metric}_N{low}'])
            if abs(row[f'{metric}_N{low}']) > _EPSILON else float('nan')
        ),
        axis=1,
    )
    return deltas.sort_values(f'{metric}_delta')


def verdict(frame, metric=STABILITY_METRIC):
    """
    Hipotezin genel sonucunu ozetler.

    Donen sozlukte:
      sonuc          : baskin siniflandirma
      oybirligi_mi   : tum kombinasyonlar ayni yonde mi
      sayimlar       : her siniftan kac kombinasyon
      ortalama_delta : ortalama degisim
    """
    evaluation = evaluate(frame, metric)
    if evaluation.empty:
        return {'sonuc': UNCHANGED, 'oybirligi_mi': False, 'sayimlar': {},
                'ortalama_delta': float('nan'), 'kombinasyon_sayisi': 0}

    counts = evaluation['sonuc'].value_counts().to_dict()
    dominant = max(counts, key=counts.get)
    return {
        'sonuc': dominant,
        'oybirligi_mi': len(counts) == 1,
        'sayimlar': counts,
        'ortalama_delta': float(evaluation[f'{metric}_delta'].mean()),
        'kombinasyon_sayisi': int(len(evaluation)),
    }


def build_report(frame):
    """Hipotez testinin Markdown raporunu uretir."""
    from swarm_bt_analysis.report_ofat import _format_table

    scales = sorted(int(n) for n in frame['N'].unique())
    low, high = scales[0], scales[-1]
    summary = verdict(frame)
    evaluation = evaluate(frame)

    lines = ['# "Manipülasyona Açıklık" Hipotez Testi\n']
    lines.append(
        'Araştırma sorusu 2: BT\'nin karar mekanizması ajan sayısı arttıkça '
        '**daha kararsız/oynak mı** (manipülasyona açık) yoksa **daha kararlı mı** '
        '(dayanıklı) hale geliyor?\n')
    lines.append(
        '**Metriğin yönü:** Bölüm 6\'daki *atama kararlılığı* metriği, bir '
        'drone\'un atanmış alanının kaç kez değiştiğini sayar. Sayaç '
        '**yükseldikçe sistem daha oynak** olur — yani metriğin adı ölçülen '
        'şeyin tersini çağrıştırır. Aşağıdaki yorumlar bu yöne göre yapılmıştır.\n')

    lines.append('## Sonuç\n')
    label = {
        MORE_STABLE: 'DAHA KARARLI (dayanıklı)',
        MORE_VOLATILE: 'DAHA OYNAK (manipülasyona açık)',
        UNCHANGED: 'DEĞİŞMEDİ',
    }[summary['sonuc']]
    unanimity = 'oybirliğiyle' if summary['oybirligi_mi'] else 'çoğunlukla'
    lines.append(
        f'Ölçek N={low} → N={high} arttığında sistem **{label}** hale geliyor '
        f'({unanimity}, {summary["kombinasyon_sayisi"]} kombinasyonun '
        f'{summary["sayimlar"].get(summary["sonuc"], 0)} tanesi).\n')
    lines.append(
        f'- Ortalama değişim: **{summary["ortalama_delta"]:+.3f}** '
        'atama değişikliği / ajan\n')

    lines.append('\n## Kombinasyon Bazında\n')
    columns = ['kombinasyon_id', f'{STABILITY_METRIC}_N{low}',
               f'{STABILITY_METRIC}_N{high}', f'{STABILITY_METRIC}_delta',
               'bagil_degisim', 'sonuc']
    lines.append(_format_table(evaluation[columns]))

    if CHURN_METRIC in frame.columns:
        churn_summary = verdict(frame, CHURN_METRIC)
        churn = evaluate(frame, CHURN_METRIC)

        lines.append('\n## İkinci Ölçüm — Churn Oranı\n')
        lines.append(
            'Churn oranı, **karşılaşma başına** değişiklik olasılığını ölçer: '
            'karşılaşmaların ne kadarı gerçek bir atama değişikliğine yol '
            'açıyor? Atama kararlılığı ise **ajan başına** toplam değişikliği '
            'sayar. İkisi farklı sorular soruyor.\n')

        if churn_summary['sonuc'] != summary['sonuc']:
            lines.append(
                '### ⚠ İki ölçüm ZIT yönde\n')
            lines.append(
                f'- **Ajan başına** atama değişikliği: {summary["sayimlar"].get(MORE_STABLE, 0)}'
                f'/{summary["kombinasyon_sayisi"]} kombinasyonda **azalıyor** '
                '→ sistem daha kararlı.\n'
                f'- **Karşılaşma başına** değişiklik olasılığı: '
                f'{churn_summary["sayimlar"].get(MORE_VOLATILE, 0)}'
                f'/{churn_summary["kombinasyon_sayisi"]} kombinasyonda '
                '**artıyor** → her tekil karşılaşma daha çok değişiklik '
                'tetikliyor.\n')
            lines.append(
                'Bu bir çelişki değil, **iki farklı ölçek etkisinin** birlikte '
                'çalışmasıdır. N arttıkça (a) iş daha çok ajana bölündüğü için '
                'ajan başına düşen yeniden-atama azalıyor, ama (b) karşılaşmalar '
                'daha yoğun bir alanda ve daha dengesiz iş yükleriyle gerçekleştiği '
                'için her tekil karşılaşmanın karar üretme olasılığı artıyor.\n')
            lines.append(
                '**Sorunun cevabı ölçüm merceğine bağlı:** sürü düzeyinde bakınca '
                'ölçekle birlikte *dayanıklılık* artıyor; tekil müzakere düzeyinde '
                'bakınca *oynaklık* artıyor. Bir saldırgan tek bir karşılaşmayı '
                'manipüle etmeye çalışıyorsa N=5\'te şansı daha yüksek; sürünün '
                'genel atama düzenini bozmaya çalışıyorsa daha düşük.\n')
        else:
            lines.append(
                'İki ölçüm **aynı yönde**: '
                f'{churn_summary["sonuc"].replace("_", " ")}.\n')

        churn_columns = ['kombinasyon_id', f'{CHURN_METRIC}_N{low}',
                         f'{CHURN_METRIC}_N{high}', f'{CHURN_METRIC}_delta', 'sonuc']
        lines.append(_format_table(churn[churn_columns]))

    return '\n'.join(lines) + '\n'


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(
        description='"Manipulasyona aciklik" hipotezini test eder.')
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
