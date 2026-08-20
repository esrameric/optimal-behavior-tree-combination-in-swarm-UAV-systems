"""Homojen <-> heterojen karsilastirmasinin birim testleri (plan Bolum 5/Faz 4)."""

import io

import pytest

from swarm_bt_analysis import cross_study, ofat


def _sweep(time_n5, stability_n5, combination='P2c_P3c_P4b_P5abc_P6c'):
    return ofat.load_sweep(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,'
        'gorev_tamamlama_suresi,atama_kararliligi,churn_orani,kapsama_dengesizligi,'
        'karsilasma_sikligi,iletisim_yuku,tick_maliyeti,carpisma_sayisi\n'
        f'A3,{combination},faz1,3,c,c,b,abc,c,10,100,1.0,0.5,2.0,10,20,1000,1\n'
        f'A5,{combination},faz1,5,c,c,b,abc,c,10,{time_n5},{stability_n5},'
        '0.4,1.6,20,40,800,2\n'
    ))


def test_ortak_kombinasyon_yoksa_bos_doner():
    homo = _sweep(80, 0.8, 'P2c_P3c_P4b_P5abc_P6c')
    hetero = _sweep(80, 0.8, 'P2a_P3a_P4a_P5a_P6a')
    assert cross_study.compare(homo, hetero).empty


def test_ayni_yonlu_olcek_etkisi_isaretlenmez():
    homo = _sweep(80, 0.8)      # sure duşuyor, kararlilik duşuyor
    hetero = _sweep(70, 0.6)    # ayni yonler
    comparison = cross_study.compare(homo, hetero)
    effective = comparison[~comparison['her_ikisi_de_etkisiz']]
    assert effective['ayni_yon'].all()


def test_zit_yonlu_olcek_etkisi_yakalanir():
    """Rol heterojenligi bir parametrenin olcek davranisini degistirebilir."""
    homo = _sweep(80, 0.8)      # kararlilik 1.0 -> 0.8 (duşuyor)
    hetero = _sweep(80, 1.4)    # kararlilik 1.0 -> 1.4 (yukseliyor)
    comparison = cross_study.compare(homo, hetero)
    row = comparison[comparison['metrik'] == 'atama_kararliligi'].iloc[0]
    assert row['delta_homojen'] == pytest.approx(-0.2)
    assert row['delta_heterojen'] == pytest.approx(0.4)
    assert not row['ayni_yon']


def test_ozet_uyum_oranini_verir():
    homo = _sweep(80, 0.8)
    hetero = _sweep(80, 1.4)
    summary = cross_study.agreement_summary(cross_study.compare(homo, hetero))
    assert summary['olculen'] > 0
    assert 0.0 <= summary['oran'] <= 1.0
    assert summary['ayni_yon'] < summary['olculen']


def test_bos_karsilastirma_ozeti_guvenli():
    import pandas as pd
    summary = cross_study.agreement_summary(pd.DataFrame())
    assert summary['olculen'] == 0


def test_rapor_ortak_kombinasyon_yoksa_acikca_soyler():
    homo = _sweep(80, 0.8, 'P2c_P3c_P4b_P5abc_P6c')
    hetero = _sweep(80, 0.8, 'P2a_P3a_P4a_P5a_P6a')
    report = cross_study.build_report(homo, hetero)
    assert 'Karşılaştırılabilir ortak kombinasyon bulunamadı' in report


def test_rapor_ayrisan_olcumleri_listeler():
    homo = _sweep(80, 0.8)
    hetero = _sweep(80, 1.4)
    report = cross_study.build_report(homo, hetero)
    assert 'zıt yönde' in report
    assert 'atama_kararliligi' in report


def test_rapor_tam_uyumu_da_soyler():
    homo = _sweep(80, 0.8)
    hetero = _sweep(70, 0.6)
    report = cross_study.build_report(homo, hetero)
    assert 'aynı yönde' in report
