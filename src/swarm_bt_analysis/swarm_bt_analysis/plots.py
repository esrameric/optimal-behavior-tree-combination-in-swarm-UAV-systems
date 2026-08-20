"""
Faz 3 gorsellestirme - plan Bolum 5/Faz 3.

Plan: "Her kombinasyon icin N=3 -> N=5 arasi metrik degisimini gorsellestir
(oncesi/sonrasi bar chart ya da slope chart)."

Slope chart secildi: her kombinasyon icin iki olcegi birlestiren bir cizgi.
Bar chart, 16 kombinasyon x 8 metrik icin okunamayacak kadar kalabalik olurdu;
slope chart ise DEGISIMIN YONUNU dogrudan gosterir -- calismanin asil sordugu
sey de bu (ayni kombinasyon olcekle nasil davraniyor).
"""

from __future__ import annotations

import argparse
import os

import matplotlib

# Bassiz calisma sart: bu scriptler CI'da ve sunucuda da koşulabilmeli.
matplotlib.use('Agg')

import matplotlib.pyplot as plt  # noqa: E402  (backend secimi importtan once olmali)

from swarm_bt_analysis import ofat  # noqa: E402

#: Grafiklerde kullanilan renkler; yon bilgisi tasir.
_IMPROVED_COLOR = '#2a7f5f'
_WORSENED_COLOR = '#b5453b'
_NEUTRAL_COLOR = '#8a8a8a'

#: Metrik icin "buyuk daha iyi mi". Gorev suresi, tick, carpisma, iletisim
#: yuku icin kucuk daha iyidir; atama kararliligi bir ODCU degil, bir
#: DAVRANIS ozelligi oldugu icin notr birakilir.
_HIGHER_IS_BETTER = {
    'gorev_tamamlama_suresi': False,
    'tick_maliyeti': False,
    'iletisim_yuku': False,
    'carpisma_sayisi': False,
    'kapsama_dengesizligi': False,
    'karsilasma_sikligi': None,
    'atama_kararliligi': None,
    'churn_orani': None,
}


def _direction_color(metric, delta):
    """Degisimin yonune gore renk secer."""
    better = _HIGHER_IS_BETTER.get(metric)
    if better is None or delta == 0:
        return _NEUTRAL_COLOR
    improved = (delta > 0) if better else (delta < 0)
    return _IMPROVED_COLOR if improved else _WORSENED_COLOR


def slope_chart(frame, metric, ax=None, label_combinations=True):
    """
    Bir metrik icin N=3 -> N=5 slope chart'i cizer.

    Her kombinasyon bir cizgi; renk degisimin iyilesme mi kotulesme mi
    oldugunu gosterir (metrik icin yon tanimliysa).
    """
    if metric not in frame.columns:
        raise ValueError(f'CSV {metric} sutununu icermiyor')

    deltas = ofat.scale_deltas(frame, [metric])
    if deltas.empty:
        raise ValueError('Karsilastirilacak iki olcek bulunamadi')

    scales = sorted(int(n) for n in frame['N'].unique())
    low, high = scales[0], scales[-1]
    low_column = f'{metric}_N{low}'
    high_column = f'{metric}_N{high}'

    if ax is None:
        _, ax = plt.subplots(figsize=(7.5, 5.5))

    # Etiketler sag uctaki degere gore siralanip ust uste binmeyecek sekilde
    # dagitilir; ham degerlere yaslanirsa yakin cizgilerin etiketleri
    # birbirinin uzerine biniyor (olculdu).
    ordered = deltas.sort_values(high_column, ascending=False).reset_index(drop=True)
    span = ordered[high_column].max() - ordered[high_column].min()
    label_step = (span / max(len(ordered) - 1, 1)) if span > 0 else 0.0
    label_top = ordered[high_column].max()

    for position, row in ordered.iterrows():
        delta = row[f'{metric}_delta']
        color = _direction_color(metric, delta)
        ax.plot([0, 1], [row[low_column], row[high_column]],
                marker='o', color=color, linewidth=1.6, markersize=5, alpha=0.85)
        if label_combinations:
            label_y = label_top - position * label_step
            ax.annotate(
                row['kombinasyon_id'],
                xy=(1, row[high_column]), xytext=(1.06, label_y),
                fontsize=6.5, color=color, va='center',
                arrowprops={'arrowstyle': '-', 'color': color,
                            'alpha': 0.35, 'linewidth': 0.6})

    ax.set_xticks([0, 1])
    ax.set_xticklabels([f'N={low}', f'N={high}'])
    ax.set_xlim(-0.15, 1.75)
    ax.set_ylabel(metric)
    ax.set_title(f'{metric}: N={low} -> N={high}', fontsize=11)
    ax.grid(axis='y', alpha=0.25, linestyle=':')
    ax.spines[['top', 'right']].set_visible(False)
    return ax


def write_slope_charts(frame, output_dir, metrics=None):
    """Her metrik icin bir slope chart dosyasi yazar; yollari dondurur."""
    metrics = metrics or ofat.CORE_METRICS
    os.makedirs(output_dir, exist_ok=True)

    written = []
    for metric in metrics:
        if metric not in frame.columns:
            continue
        figure, ax = plt.subplots(figsize=(7.5, 5.5))
        slope_chart(frame, metric, ax=ax)
        figure.tight_layout()
        path = os.path.join(output_dir, f'slope_{metric}.png')
        figure.savefig(path, dpi=140)
        plt.close(figure)
        written.append(path)
    return written


def sensitivity_bar_chart(frame, output_path, top=10):
    """
    Kombinasyonlari ortalama N-duyarliligina gore siralayan yatay bar chart.

    Plan Bolum 5/Faz 3: "Hangi parametrelerin N-duyarliligi en yuksek/dusuk
    oldugunu belirle."
    """
    sensitivity = ofat.n_sensitivity(frame).head(top)
    if sensitivity.empty:
        raise ValueError('N-duyarlilik hesaplanamadi')

    figure, ax = plt.subplots(figsize=(8.5, 0.42 * len(sensitivity) + 1.6))
    labels = list(sensitivity['kombinasyon_id'])[::-1]
    values = list(sensitivity['ortalama_duyarlilik'])[::-1]
    ax.barh(labels, values, color='#3a6ea5', alpha=0.9)
    ax.set_xlabel('ortalama N-duyarlilik skoru')
    ax.set_title('Olcek duyarliligi siralamasi (|N=5 - N=3| / N=3)', fontsize=11)
    ax.grid(axis='x', alpha=0.25, linestyle=':')
    ax.spines[['top', 'right']].set_visible(False)
    ax.tick_params(axis='y', labelsize=7.5)
    figure.tight_layout()
    figure.savefig(output_path, dpi=140)
    plt.close(figure)
    return output_path


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(
        description='Faz 3 gorsellerini uretir (slope chart + duyarlilik siralamasi).')
    parser.add_argument('csv', help='ofat_sweep ciktisi (CSV)')
    parser.add_argument('-o', '--output-dir', default='experiments/figures',
                        help='gorsellerin yazilacagi dizin')
    args = parser.parse_args(argv)

    frame = ofat.load_sweep(args.csv)
    paths = write_slope_charts(frame, args.output_dir)
    paths.append(sensitivity_bar_chart(
        frame, os.path.join(args.output_dir, 'n_duyarlilik_siralamasi.png')))

    for path in paths:
        print(path)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
