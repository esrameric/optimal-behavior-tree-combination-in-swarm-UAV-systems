"""Faz 1 <-> Faz 2 karsilastirmasinin birim testleri (plan Bolum 5/Faz 2)."""

import io

import pandas as pd
import pytest

from swarm_bt_analysis import phase_comparison as pc


def _campaign_csv():
    """
    Iki faz x iki kombinasyon x iki olcek iceren asgari kampanya tablosu.

    A kombinasyonu: iki faz da AYNI yonde (karsilasma artiyor, sure duşuyor).
    B kombinasyonu: churn metriginde iki faz ZIT yonde -- kod-seviyesi model
    Gazebo'yu o metrikte yanlis tahmin ediyor.
    """
    return io.StringIO(
        'deney_id,kombinasyon_id,faz,N,tohum,gorev_suresi,tick,kapsama_tamam,'
        'karsilasma,takas,devralinan,churn_orani,atama_kararliligi,carpisma\n'
        'A_N3,A,kod,3,0,100,1000,1,2,0,0,0.20,0.5,0\n'
        'A_N5,A,kod,5,0,60,600,1,6,1,0,0.40,0.8,1\n'
        'A_N3,A,gazebo,3,0,105,1050,1,2,0,0,0.20,0.5,0\n'
        'A_N5,A,gazebo,5,0,64,640,1,7,1,0,0.45,0.9,1\n'
        'B_N3,B,kod,3,0,90,900,1,1,0,0,0.30,0.4,0\n'
        'B_N5,B,kod,5,0,55,550,1,4,0,0,0.50,0.7,0\n'
        'B_N3,B,gazebo,3,0,95,950,1,1,0,0,0.60,0.4,0\n'
        'B_N5,B,gazebo,5,0,58,580,1,5,0,0,0.30,0.7,0\n'
    )


@pytest.fixture()
def campaign():
    return pc.load(_campaign_csv())


def test_eksik_sutun_hata_verir():
    with pytest.raises(ValueError, match='eksik sutun'):
        pc.load(io.StringIO('deney_id,faz\nA,kod\n'))


def test_tamamlanma_raporu_her_faz_ve_olcek_icin_satir_uretir(campaign):
    report = pc.completion_report(campaign)
    assert len(report) == 8          # 2 faz x 2 kombinasyon x 2 olcek
    assert (report['tamamlanma_orani'] == 1.0).all()
    assert (report['tekrar'] == 1).all()


def test_karsilasma_sikligi_olcekle_artiyor(campaign):
    """Plan Bolum 5/Faz 2: N=5'te beklenen artis dogrulanmali."""
    scaling = pc.encounter_scaling(campaign)
    assert len(scaling) == 4         # 2 faz x 2 kombinasyon
    assert scaling['artti_mi'].all()

    row = scaling[
        (scaling['faz'] == 'kod') & (scaling['kombinasyon_id'] == 'A')
    ].iloc[0]
    assert row['karsilasma_N3'] == pytest.approx(2.0)
    assert row['karsilasma_N5'] == pytest.approx(6.0)
    assert row['artis_katsayisi'] == pytest.approx(3.0)


def test_ayni_yonlu_metrikler_isaretlenmez(campaign):
    agreement = pc.scale_direction_agreement(campaign)
    row = agreement[
        (agreement['kombinasyon_id'] == 'A') & (agreement['metrik'] == 'gorev_suresi')
    ].iloc[0]
    assert row['delta_kod'] < 0 and row['delta_gazebo'] < 0
    assert row['ayni_yon']


def test_zit_yonlu_metrik_yakalanir(campaign):
    """Kod-seviyesi model bir metrikte Gazebo'yu ters tahmin ederse gorunmeli."""
    agreement = pc.scale_direction_agreement(campaign)
    row = agreement[
        (agreement['kombinasyon_id'] == 'B') & (agreement['metrik'] == 'churn_orani')
    ].iloc[0]
    assert row['delta_kod'] == pytest.approx(0.20)
    assert row['delta_gazebo'] == pytest.approx(-0.30)
    assert not row['ayni_yon']


def test_iki_tarafta_da_etkisiz_ayri_isaretlenir(campaign):
    agreement = pc.scale_direction_agreement(campaign)
    row = agreement[
        (agreement['kombinasyon_id'] == 'B') & (agreement['metrik'] == 'takas')
    ].iloc[0]
    assert row['her_ikisi_de_etkisiz']


def test_bagil_sapma_hesaplanir(campaign):
    gap = pc.phase_gap(campaign)
    row = gap[(gap['kombinasyon_id'] == 'A') & (gap['N'] == 3)].iloc[0]
    assert row['gorev_suresi_kod'] == pytest.approx(100.0)
    assert row['gorev_suresi_gazebo'] == pytest.approx(105.0)
    assert row['gorev_suresi_bagil_sapma'] == pytest.approx(0.05)


def test_sifir_degerli_metrikte_sapma_tanimsiz(campaign):
    gap = pc.phase_gap(campaign)
    row = gap[(gap['kombinasyon_id'] == 'A') & (gap['N'] == 3)].iloc[0]
    assert pd.isna(row['carpisma_bagil_sapma'])


def test_tek_fazli_kampanya_karsilastirma_uretmez():
    frame = pd.DataFrame({
        'deney_id': ['A_N3', 'A_N5'], 'kombinasyon_id': ['A', 'A'],
        'faz': ['kod', 'kod'], 'N': [3, 5], 'tohum': [0, 0],
        'gorev_suresi': [100.0, 50.0], 'kapsama_tamam': [1, 1],
        'karsilasma': [2, 5],
    })
    assert pc.scale_direction_agreement(frame).empty
    assert pc.phase_gap(frame).empty


def test_rapor_dort_bolum_icerir(campaign):
    report = pc.build_report(campaign)
    assert '# Faz 1 ↔ Faz 2 Karşılaştırması' in report
    assert '## 1. Tamamlanma' in report
    assert '## 2. Karşılaşma Sıklığı' in report
    assert '## 3. İki Fazın N-Duyarlılığı' in report
    assert '## 4. Model Doğruluğu' in report
