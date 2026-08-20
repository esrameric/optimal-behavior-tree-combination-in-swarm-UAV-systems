"""OFAT olcek-duyarlilik analizinin birim testleri."""

import io

import pandas as pd
import pytest

from swarm_bt_analysis import ofat


def _sweep_csv():
    """
    Iki kombinasyon x iki olcek iceren asgari bir tarama tablosu.

    P4 ekseni bilincli olarak N'e duyarli kurgulandi: N=3'te gorev suresini
    dusururken N=5'te yukseltiyor.

    """
    return io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,'
        'gorev_tamamlama_suresi,atama_kararliligi,churn_orani,kapsama_dengesizligi,'
        'karsilasma_sikligi,iletisim_yuku,tick_maliyeti,carpisma_sayisi\n'
        'B_N3,B,faz1,3,c,c,b,abc,c,10,100,1.0,0.5,2.0,10,20,1000,1\n'
        'B_N5,B,faz1,5,c,c,b,abc,c,10,50,0.5,0.25,1.0,20,40,500,2\n'
        'X_N3,X,faz1,3,c,c,c,abc,c,10,90,1.2,0.6,2.0,10,20,900,1\n'
        'X_N5,X,faz1,5,c,c,c,abc,c,10,60,0.4,0.20,1.0,20,40,600,2\n'
    )


@pytest.fixture()
def sweep():
    return ofat.load_sweep(_sweep_csv())


def test_eksik_sutun_hata_verir():
    bozuk = io.StringIO('deney_id,N\nA,3\n')
    with pytest.raises(ValueError, match='eksik sutun'):
        ofat.load_sweep(bozuk)


def test_baseline_ilk_satirdan_alinir(sweep):
    assert ofat.baseline_combination(sweep) == 'B'


def test_degisen_eksen_bulunur(sweep):
    baseline = sweep.iloc[0]
    assert ofat.changed_axis(baseline, sweep.iloc[2]) == 'P4'
    assert ofat.changed_axis(baseline, baseline) == 'baseline'


def test_etki_tablosu_baseline_satirini_dislar(sweep):
    effects = ofat.effect_table(sweep)
    assert set(effects['kombinasyon_id']) == {'X'}
    assert len(effects) == 2  # X x {N=3, N=5}


def test_etki_tablosu_deltalari_dogru(sweep):
    effects = ofat.effect_table(sweep)
    n3 = effects[effects['N'] == 3].iloc[0]
    n5 = effects[effects['N'] == 5].iloc[0]

    assert n3['gorev_tamamlama_suresi_delta'] == pytest.approx(-10.0)
    assert n5['gorev_tamamlama_suresi_delta'] == pytest.approx(10.0)
    assert n3['gorev_tamamlama_suresi_delta_orani'] == pytest.approx(-0.10)
    assert n5['gorev_tamamlama_suresi_delta_orani'] == pytest.approx(0.20)


def test_yon_uyusmazligi_tespit_edilir(sweep):
    """Plan Bolum 4'un aradigi bulgu: etki iki N'de zit yonde."""
    agreement = ofat.direction_agreement(sweep)
    row = agreement[
        (agreement['kombinasyon_id'] == 'X')
        & (agreement['metrik'] == 'gorev_tamamlama_suresi')
    ].iloc[0]

    assert row['degisen_eksen'] == 'P4'
    assert row['delta_N3'] == pytest.approx(-10.0)
    assert row['delta_N5'] == pytest.approx(10.0)
    assert not row['ayni_yon']


def test_ayni_yonlu_etki_isaretlenmez(sweep):
    agreement = ofat.direction_agreement(sweep)
    row = agreement[
        (agreement['kombinasyon_id'] == 'X')
        & (agreement['metrik'] == 'atama_kararliligi')
    ].iloc[0]
    # N=3: 1.2-1.0 = +0.2 ; N=5: 0.4-0.5 = -0.1  -> zit yon
    assert not row['ayni_yon']

    row = agreement[
        (agreement['kombinasyon_id'] == 'X')
        & (agreement['metrik'] == 'tick_maliyeti')
    ].iloc[0]
    # N=3: 900-1000 = -100 ; N=5: 600-500 = +100 -> zit yon
    assert not row['ayni_yon']

    row = agreement[
        (agreement['kombinasyon_id'] == 'X')
        & (agreement['metrik'] == 'karsilasma_sikligi')
    ].iloc[0]
    # Ikisinde de fark yok -> ayni yon (sifir) ve etkisiz
    assert row['ayni_yon']
    assert row['her_ikisi_de_etkisiz']


def test_n_duyarlilik_skoru(sweep):
    """Bolum 6: |N=5 degeri - N=3 degeri| / N=3 degeri."""
    sensitivity = ofat.n_sensitivity(sweep).set_index('kombinasyon_id')

    # Baseline: gorev suresi 100 -> 50, skor = 50/100 = 0.5
    assert sensitivity.loc['B', 'gorev_tamamlama_suresi'] == pytest.approx(0.5)
    # X: 90 -> 60, skor = 30/90 = 0.333...
    assert sensitivity.loc['X', 'gorev_tamamlama_suresi'] == pytest.approx(1 / 3)
    assert 'ortalama_duyarlilik' in sensitivity.columns


def test_n_duyarlilik_azalan_sirada(sweep):
    sensitivity = ofat.n_sensitivity(sweep)
    values = list(sensitivity['ortalama_duyarlilik'])
    assert values == sorted(values, reverse=True)


def test_olcek_deltalari_ham_degerleri_tasir(sweep):
    deltas = ofat.scale_deltas(sweep).set_index('kombinasyon_id')
    assert deltas.loc['B', 'gorev_tamamlama_suresi_N3'] == pytest.approx(100.0)
    assert deltas.loc['B', 'gorev_tamamlama_suresi_N5'] == pytest.approx(50.0)
    assert deltas.loc['B', 'gorev_tamamlama_suresi_delta'] == pytest.approx(-50.0)


def test_sifir_paydali_metrik_nan_verir():
    frame = pd.DataFrame({
        'deney_id': ['A_N3', 'A_N5'],
        'kombinasyon_id': ['A', 'A'],
        'N': [3, 5],
        'P2': ['c', 'c'], 'P3': ['c', 'c'], 'P4': ['b', 'b'],
        'P5': ['abc', 'abc'], 'P6': ['c', 'c'],
        'gorev_tamamlama_suresi': [0.0, 5.0],
    })
    sensitivity = ofat.n_sensitivity(frame, metrics=['gorev_tamamlama_suresi'])
    assert pd.isna(sensitivity.iloc[0]['gorev_tamamlama_suresi'])


def test_tek_olcekli_tarama_bos_doner():
    frame = pd.DataFrame({
        'deney_id': ['A_N3'], 'kombinasyon_id': ['A'], 'N': [3],
        'P2': ['c'], 'P3': ['c'], 'P4': ['b'], 'P5': ['abc'], 'P6': ['c'],
        'gorev_tamamlama_suresi': [10.0],
    })
    assert ofat.n_sensitivity(frame, metrics=['gorev_tamamlama_suresi']).empty


def test_zit_yon_ile_tek_tarafli_etki_ayrilir(sweep):
    """Gercek catisma ile "bir olcekte etki yok" durumu ayni sey degildir."""
    agreement = ofat.direction_agreement(sweep)

    # gorev suresi: -10 (N=3) vs +10 (N=5) -> gercek catisma
    row = agreement[agreement['metrik'] == 'gorev_tamamlama_suresi'].iloc[0]
    assert row['zit_yon']
    assert not row['tek_tarafli_etki']

    # kapsama_dengesizligi: iki tarafta da 0 -> ne catisma ne esik
    row = agreement[agreement['metrik'] == 'kapsama_dengesizligi'].iloc[0]
    assert not row['zit_yon']
    assert not row['tek_tarafli_etki']
    assert row['her_ikisi_de_etkisiz']


def test_tek_tarafli_etki_isaretlenir():
    """N=3'te etki yok, N=5'te var: olcek esigi, catisma degil."""
    frame = pd.DataFrame({
        'deney_id': ['B_N3', 'B_N5', 'X_N3', 'X_N5'],
        'kombinasyon_id': ['B', 'B', 'X', 'X'],
        'N': [3, 5, 3, 5],
        'P2': ['c'] * 4, 'P3': ['c'] * 4, 'P4': ['b', 'b', 'c', 'c'],
        'P5': ['abc'] * 4, 'P6': ['c'] * 4,
        'gorev_tamamlama_suresi': [100.0, 50.0, 100.0, 55.0],
    })
    agreement = ofat.direction_agreement(frame, metrics=['gorev_tamamlama_suresi'])
    row = agreement.iloc[0]
    assert row['delta_N3'] == pytest.approx(0.0)
    assert row['delta_N5'] == pytest.approx(5.0)
    assert not row['zit_yon']
    assert row['tek_tarafli_etki']
