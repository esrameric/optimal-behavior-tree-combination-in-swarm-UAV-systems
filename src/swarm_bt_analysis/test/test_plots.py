"""Faz 3 gorsellestirmesinin birim testleri (plan Bolum 5/Faz 3)."""

import io
import os

import pytest

from swarm_bt_analysis import ofat, plots


def _sweep():
    return ofat.load_sweep(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,'
        'gorev_tamamlama_suresi,atama_kararliligi,churn_orani,kapsama_dengesizligi,'
        'karsilasma_sikligi,iletisim_yuku,tick_maliyeti,carpisma_sayisi\n'
        'B_N3,B,faz1,3,c,c,b,abc,c,10,100,1.0,0.5,2.0,10,20,1000,1\n'
        'B_N5,B,faz1,5,c,c,b,abc,c,10,50,0.5,0.25,1.0,20,40,500,2\n'
        'X_N3,X,faz1,3,c,c,c,abc,c,10,90,1.2,0.6,2.0,10,20,900,1\n'
        'X_N5,X,faz1,5,c,c,c,abc,c,10,60,0.4,0.20,1.0,20,40,600,2\n'
    ))


@pytest.fixture()
def sweep():
    return _sweep()


def test_bassiz_backend_secili():
    """Grafikler sunucuda/CI'da da uretilebilmeli."""
    import matplotlib
    assert matplotlib.get_backend().lower() == 'agg'


def test_slope_chart_her_kombinasyon_icin_cizgi_cizer(sweep):
    ax = plots.slope_chart(sweep, 'gorev_tamamlama_suresi')
    # Her kombinasyon icin bir cizgi (2 kombinasyon).
    assert len(ax.get_lines()) == 2
    assert [t.get_text() for t in ax.get_xticklabels()] == ['N=3', 'N=5']


def test_slope_chart_bilinmeyen_metrigi_reddeder(sweep):
    with pytest.raises(ValueError, match='sutununu icermiyor'):
        plots.slope_chart(sweep, 'olmayan_metrik')


def test_yon_rengi_iyilesme_ve_kotulesmeyi_ayirir():
    # Gorev suresi icin kucuk daha iyi: negatif delta iyilesme.
    improved = plots._direction_color('gorev_tamamlama_suresi', -10)
    worsened = plots._direction_color('gorev_tamamlama_suresi', +10)
    assert improved != worsened

    # Atama kararliligi bir odcu degil, davranis ozelligi -> notr.
    assert plots._direction_color('atama_kararliligi', -0.5) == plots._NEUTRAL_COLOR
    assert plots._direction_color('atama_kararliligi', +0.5) == plots._NEUTRAL_COLOR


def test_sifir_delta_notr_renkli():
    assert plots._direction_color('gorev_tamamlama_suresi', 0) == plots._NEUTRAL_COLOR


def test_her_metrik_icin_dosya_yazilir(sweep, tmp_path):
    paths = plots.write_slope_charts(sweep, str(tmp_path))
    assert len(paths) == len(ofat.CORE_METRICS)
    for path in paths:
        assert os.path.exists(path)
        assert os.path.getsize(path) > 0


def test_duyarlilik_bar_charti_yazilir(sweep, tmp_path):
    path = plots.sensitivity_bar_chart(sweep, str(tmp_path / 'siralama.png'))
    assert os.path.exists(path)
    assert os.path.getsize(path) > 0


def test_tek_olcekli_tarama_slope_chart_uretemez():
    import pandas as pd
    frame = pd.DataFrame({
        'deney_id': ['A_N3'], 'kombinasyon_id': ['A'], 'N': [3], 'tekrar': [10],
        'P2': ['c'], 'P3': ['c'], 'P4': ['b'], 'P5': ['abc'], 'P6': ['c'],
        'gorev_tamamlama_suresi': [10.0],
    })
    with pytest.raises(ValueError, match='iki olcek'):
        plots.slope_chart(frame, 'gorev_tamamlama_suresi')


def test_ana_akis_tum_gorselleri_uretir(sweep, tmp_path):
    csv_path = tmp_path / 'tarama.csv'
    sweep.to_csv(csv_path, index=False)

    plots.main([str(csv_path), '-o', str(tmp_path / 'figures')])
    written = os.listdir(tmp_path / 'figures')
    assert 'n_duyarlilik_siralamasi.png' in written
    assert any(name.startswith('slope_') for name in written)
