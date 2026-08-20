"""Metrik tablosu uretiminin birim testleri (plan Bolum 5/Faz 1)."""

import io

import pandas as pd
import pytest

from swarm_bt_analysis import metrics_table, ofat


@pytest.fixture()
def sweep():
    return ofat.load_sweep(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,'
        'gorev_tamamlama_suresi,atama_kararliligi,churn_orani,kapsama_dengesizligi,'
        'karsilasma_sikligi,iletisim_yuku,tick_maliyeti,carpisma_sayisi\n'
        'B_N3,B,faz1,3,c,c,b,abc,c,10,100,1.0,0.5,2.0,10,20,1000,1\n'
        'B_N5,B,faz1,5,c,c,b,abc,c,10,50,0.5,0.25,1.0,20,40,500,2\n'
        'X_N3,X,faz1,3,c,c,c,abc,c,10,90,1.2,0.6,2.0,10,20,900,1\n'
        'X_N5,X,faz1,5,c,c,c,abc,c,10,60,0.4,0.20,1.0,20,40,600,2\n'
    ))


def test_tablo_her_kombinasyon_icin_bir_satir(sweep):
    table = metrics_table.build_table(sweep)
    assert set(table['kombinasyon_id']) == {'B', 'X'}
    assert len(table) == 2


def test_delta_sutunlari_dolduruluyor(sweep):
    """Plan Bolum 5/Faz 1 acikca N=3 -> N=5 delta sutununu istiyor."""
    table = metrics_table.build_table(sweep).set_index('kombinasyon_id')

    assert table.loc['B', 'gorev_tamamlama_suresi_N3'] == pytest.approx(100.0)
    assert table.loc['B', 'gorev_tamamlama_suresi_N5'] == pytest.approx(50.0)
    assert table.loc['B', 'gorev_tamamlama_suresi_delta'] == pytest.approx(-50.0)
    assert table.loc['X', 'gorev_tamamlama_suresi_delta'] == pytest.approx(-30.0)


def test_duyarlilik_sutunlari_eklenmis(sweep):
    table = metrics_table.build_table(sweep).set_index('kombinasyon_id')
    assert table.loc['B', 'gorev_tamamlama_suresi_duyarlilik'] == pytest.approx(0.5)
    assert 'ortalama_duyarlilik' in table.columns


def test_sutun_sirasi_metrik_bazinda_gruplu(sweep):
    table = metrics_table.build_table(sweep)
    columns = list(table.columns)
    assert columns[0] == 'kombinasyon_id'
    assert columns[1:5] == [
        'gorev_tamamlama_suresi_N3',
        'gorev_tamamlama_suresi_N5',
        'gorev_tamamlama_suresi_delta',
        'gorev_tamamlama_suresi_duyarlilik',
    ]


def test_tablo_duyarliliga_gore_azalan_sirali(sweep):
    values = list(metrics_table.build_table(sweep)['ortalama_duyarlilik'])
    assert values == sorted(values, reverse=True)


def test_markdown_ozeti_her_metrik_icin_bolum_uretir(sweep):
    text = metrics_table.build_markdown(sweep)
    assert '# Metrik Tablosu' in text
    for metric in ofat.CORE_METRICS:
        assert f'## {metric}' in text
    assert '| N=3 ' in text and '| N=5 ' in text and '| delta ' in text


def test_markdown_delta_isaretli_yazilir(sweep):
    text = metrics_table.build_markdown(sweep)
    assert '-50.000' in text     # B kombinasyonunun gorev suresi deltasi


def test_yetersiz_tekrar_ana_akista_reddedilir(tmp_path):
    path = tmp_path / 'az_tekrar.csv'
    path.write_text(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,gorev_tamamlama_suresi\n'
        'A_N3,A,faz1,3,c,c,b,abc,c,3,100\n'
        'A_N5,A,faz1,5,c,c,b,abc,c,3,50\n',
        encoding='utf-8')
    with pytest.raises(ValueError, match='tekrarin altinda'):
        metrics_table.main([str(path)])


def test_tek_olcekli_tarama_bos_tablo_verir():
    frame = pd.DataFrame({
        'deney_id': ['A_N3'], 'kombinasyon_id': ['A'], 'N': [3], 'tekrar': [10],
        'P2': ['c'], 'P3': ['c'], 'P4': ['b'], 'P5': ['abc'], 'P6': ['c'],
        'gorev_tamamlama_suresi': [10.0],
    })
    assert metrics_table.build_table(frame, metrics=['gorev_tamamlama_suresi']).empty
