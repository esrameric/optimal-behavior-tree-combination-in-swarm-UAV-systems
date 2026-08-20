"""Parametre ekseni duyarlilik analizinin birim testleri (plan Bolum 5/Faz 3)."""

import io

import pytest

from swarm_bt_analysis import ofat, parameter_sensitivity as ps


def _sweep():
    """
    Baseline + uc OFAT varyanti (P2, P3, P4 eksenlerinde birer degisiklik).

    P3 varyanti bilincli olarak en yuksek duyarlilikta kurgulandi.
    """
    return ofat.load_sweep(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,'
        'gorev_tamamlama_suresi,atama_kararliligi,churn_orani,kapsama_dengesizligi,'
        'karsilasma_sikligi,iletisim_yuku,tick_maliyeti,carpisma_sayisi\n'
        # baseline P2c_P3c_P4b_P5abc_P6c
        'B3,P2c_P3c_P4b_P5abc_P6c,faz1,3,c,c,b,abc,c,10,100,1.0,0.5,2.0,10,20,1000,2\n'
        'B5,P2c_P3c_P4b_P5abc_P6c,faz1,5,c,c,b,abc,c,10,80,0.8,0.4,1.6,12,24,800,2\n'
        # P2 varyanti (tam merkezi) - N=3'te iyi, N=5'te kotu
        'A3,P2a_P3c_P4b_P5abc_P6c,faz1,3,a,c,b,abc,c,10,95,1.0,0.5,2.0,10,20,950,2\n'
        'A5,P2a_P3c_P4b_P5abc_P6c,faz1,5,a,c,b,abc,c,10,90,0.9,0.45,1.8,11,22,900,2\n'
        # P3 varyanti - en buyuk olcek degisimi
        'C3,P2c_P3b_P4b_P5abc_P6c,faz1,3,c,b,b,abc,c,10,100,1.0,0.5,2.0,10,20,1000,2\n'
        'C5,P2c_P3b_P4b_P5abc_P6c,faz1,5,c,b,b,abc,c,10,20,0.2,0.1,0.4,40,80,200,8\n'
        # P4 varyanti
        'D3,P2c_P3c_P4c_P5abc_P6c,faz1,3,c,c,c,abc,c,10,100,1.0,0.5,2.0,10,20,1000,2\n'
        'D5,P2c_P3c_P4c_P5abc_P6c,faz1,5,c,c,c,abc,c,10,85,0.85,0.42,1.7,11,22,850,2\n'
    ))


@pytest.fixture()
def sweep():
    return _sweep()


def test_p5_secenek_adi_harf_harf_cozulur():
    assert ps.option_name('P5', 'abc') == 'dogrudan mesaj + stigmerji + intent yayini'
    assert ps.option_name('P5', 'b') == 'stigmerji'
    assert ps.option_name('P5', 'none') == 'iletisim yok'


def test_bilinen_eksen_secenekleri_adlandirilir():
    assert ps.option_name('P2', 'a') == 'tam merkezi'
    assert ps.option_name('P3', 'b') == 'Contract Net'
    assert ps.option_name('P4', 'c') == 'olay-gudumlu BT'


def test_bilinmeyen_secenek_harfe_geri_duşer():
    assert ps.option_name('P9', 'z') == 'P9z'


def test_her_varyant_degistirdigi_eksene_atfedilir(sweep):
    variants = ps.per_variant(sweep)
    mapping = dict(zip(variants['kombinasyon_id'], variants['eksen']))
    assert mapping['P2a_P3c_P4b_P5abc_P6c'] == 'P2'
    assert mapping['P2c_P3b_P4b_P5abc_P6c'] == 'P3'
    assert mapping['P2c_P3c_P4c_P5abc_P6c'] == 'P4'


def test_baseline_varyant_listesinde_yok(sweep):
    variants = ps.per_variant(sweep)
    assert 'P2c_P3c_P4b_P5abc_P6c' not in set(variants['kombinasyon_id'])


def test_eksen_siralamasi_duyarliliga_gore_azalan(sweep):
    summary = ps.by_axis(sweep)
    values = list(summary['ortalama'])
    assert values == sorted(values, reverse=True)
    # Kurguda P3 en buyuk degisimi yasiyor.
    assert summary.iloc[0]['eksen'] == 'P3'


def test_secenek_siralamasi_azalan(sweep):
    options = ps.by_option(sweep)
    values = list(options['duyarlilik'])
    assert values == sorted(values, reverse=True)


def test_mimari_karsilastirmasi_tum_p2_seceneklerini_icerir(sweep):
    comparison = ps.architecture_comparison(sweep)
    assert set(comparison['P2']) == {'a', 'c'}
    row = comparison[comparison['P2'] == 'a'].iloc[0]
    assert row['gorev_tamamlama_suresi_N3'] == pytest.approx(95.0)
    assert row['gorev_tamamlama_suresi_N5'] == pytest.approx(90.0)


def test_mimari_karsilastirmasi_baseline_farkini_verir(sweep):
    comparison = ps.architecture_comparison(sweep)
    central = comparison[comparison['P2'] == 'a'].iloc[0]
    # N=3'te merkezi baseline'dan iyi (-5), N=5'te kotu (+10).
    assert central['baseline_farki_N3'] == pytest.approx(-5.0)
    assert central['baseline_farki_N5'] == pytest.approx(10.0)


def test_rapor_yon_degisimini_acikca_soyler(sweep):
    """Planin acik sorusu bir cumleyle cevaplanmali."""
    report = ps.build_report(sweep)
    assert 'Merkezi koordinasyon ölçekle bozuluyor' in report
    assert 'En duyarlı eksen: P3' in report


def test_rapor_eksik_metrik_ile_hata_verir(sweep):
    with pytest.raises(ValueError, match='sutununu icermiyor'):
        ps.architecture_comparison(sweep, metric='olmayan')
