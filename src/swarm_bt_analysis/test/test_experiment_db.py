"""Ortak deney veritabaninin birim testleri (plan Bolum 8)."""

import io

import pandas as pd
import pytest

from swarm_bt_analysis import experiment_db as db, ofat


def _sweep():
    return ofat.load_sweep(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,'
        'gorev_tamamlama_suresi,atama_kararliligi,churn_orani,kapsama_dengesizligi,'
        'karsilasma_sikligi,iletisim_yuku,tick_maliyeti,carpisma_sayisi\n'
        'B_N3,B,faz1,3,c,c,b,abc,c,10,100,1.0,0.5,2.0,10,20,1000,1\n'
        'B_N5,B,faz1,5,c,c,b,abc,c,10,50,0.5,0.25,1.0,20,40,500,2\n'
    ))


def test_sema_plan_sablonunu_kapsar():
    """Plan Bolum 8'in istedigi tum sutunlar semada olmali."""
    for column in ['deney_id', 'tarih', 'faz', 'N', 'P2', 'P3', 'P4', 'P5', 'P6',
                   'gorev_tamamlama_suresi', 'atama_kararliligi', 'churn_orani',
                   'kapsama_dengesizligi', 'karsilasma_sikligi', 'iletisim_yuku',
                   'tick_maliyeti', 'carpisma_sayisi', 'notlar']:
        assert column in db.SCHEMA, f'{column} semada yok'


def test_calisma_sutunu_ortak_veritabanini_ayirir():
    """Iki calisma tek tabloda tutulacaksa ayirt edici bir sutun sart."""
    assert 'calisma' in db.IDENTITY_COLUMNS
    assert db.STUDY_HOMOGENEOUS != db.STUDY_HETEROGENEOUS


def test_bos_veritabani_sema_ile_uyumlu():
    assert list(db.empty_database().columns) == db.SCHEMA


def test_taramadan_donuşturme_sema_sutunlarini_uretir():
    rows = db.from_sweep(_sweep(), date='2026-08-20')
    assert list(rows.columns) == db.SCHEMA
    assert (rows['calisma'] == db.STUDY_HOMOGENEOUS).all()
    assert (rows['tarih'] == '2026-08-20').all()
    assert len(rows) == 2


def test_taramada_olmayan_sutunlar_bos_kalir():
    rows = db.from_sweep(_sweep())
    assert rows['notlar'].isna().all() or (rows['notlar'] == '').all()


def test_dogrulama_eksik_sutunu_yakalar():
    frame = pd.DataFrame({'deney_id': ['A'], 'faz': ['faz1']})
    issues = db.validate(frame)
    assert any('eksik kimlik' in issue for issue in issues)


def test_dogrulama_bilinmeyen_calisma_etiketini_yakalar():
    rows = db.from_sweep(_sweep())
    rows.loc[0, 'calisma'] = 'baska_calisma'
    assert any('bilinmeyen calisma' in issue for issue in db.validate(rows))


def test_dogrulama_gecersiz_n_yakalar():
    rows = db.from_sweep(_sweep())
    rows.loc[0, 'N'] = 0
    assert any('N pozitif' in issue for issue in db.validate(rows))


def test_dogrulama_tekrarli_anahtari_yakalar():
    rows = pd.concat([db.from_sweep(_sweep())] * 2, ignore_index=True)
    assert any('tekrarli' in issue for issue in db.validate(rows))


def test_gecerli_tablo_sorunsuz():
    assert db.validate(db.from_sweep(_sweep())) == []


def test_ekleme_bos_veritabanina_yazar():
    rows = db.from_sweep(_sweep())
    database = db.append(db.empty_database(), rows)
    assert len(database) == 2
    assert list(database.columns) == db.SCHEMA


def test_ayni_deney_tekrar_koşulunca_satir_cogalmaz():
    """Bir tarama tekrar koşulmasi veritabanini sisirmemeli."""
    rows = db.from_sweep(_sweep())
    database = db.append(db.empty_database(), rows)
    database = db.append(database, rows)
    assert len(database) == 2


def test_yeniden_koşu_eski_satiri_gunceller():
    rows = db.from_sweep(_sweep())
    database = db.append(db.empty_database(), rows)

    updated = rows.copy()
    updated['gorev_tamamlama_suresi'] = [111.0, 55.0]
    database = db.append(database, updated)

    assert len(database) == 2
    assert set(database['gorev_tamamlama_suresi']) == {111.0, 55.0}


def test_iki_calisma_yan_yana_durabilir():
    homogeneous = db.from_sweep(_sweep(), study=db.STUDY_HOMOGENEOUS)
    heterogeneous = db.from_sweep(_sweep(), study=db.STUDY_HETEROGENEOUS)

    database = db.append(db.append(db.empty_database(), homogeneous), heterogeneous)
    assert len(database) == 4
    assert set(database['calisma']) == {db.STUDY_HOMOGENEOUS, db.STUDY_HETEROGENEOUS}


def test_sema_disi_tablo_eklenemez():
    with pytest.raises(ValueError, match='sema uyumsuzlugu'):
        db.append(db.empty_database(), pd.DataFrame({'deney_id': ['A']}))


def test_ozet_calisma_faz_ve_olcek_kirilimini_verir():
    database = db.append(db.empty_database(), db.from_sweep(_sweep()))
    summary = db.summary(database)
    assert set(summary.columns) == {'calisma', 'faz', 'N', 'satir'}
    assert summary['satir'].sum() == 2


def test_olmayan_dosya_bos_veritabani_verir(tmp_path):
    assert db.load(str(tmp_path / 'yok.csv')).empty


def test_kaydet_ve_oku_gidis_donuş(tmp_path):
    path = str(tmp_path / 'db.csv')
    database = db.append(db.empty_database(), db.from_sweep(_sweep()))
    db.save(database, path)
    restored = db.load(path)
    assert list(restored.columns) == db.SCHEMA
    assert len(restored) == 2


def test_faz2_kisa_sutun_adlari_semaya_eslenir():
    """Iki faz ayni sutun kumesinde bulusmali (kisa adlar eslenir)."""
    phase2 = pd.read_csv(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,tohum,gorev_suresi,tick,kapsama_tamam,'
        'karsilasma,takas,devralinan,churn_orani,atama_kararliligi,carpisma\n'
        'B_N3,B,gazebo,3,0,36.5,365,1,2,1,0,0.5,0.667,0\n'
    ))
    phase2['tekrar'] = 1
    phase2['P2'] = 'c'
    phase2['P3'] = 'c'
    phase2['P4'] = 'b'
    phase2['P5'] = 'abc'
    phase2['P6'] = 'c'

    rows = db.from_sweep(phase2)
    assert rows['gorev_tamamlama_suresi'].iloc[0] == pytest.approx(36.5)
    assert rows['tick_maliyeti'].iloc[0] == 365
    assert rows['karsilasma_sikligi'].iloc[0] == 2
    assert rows['carpisma_sayisi'].iloc[0] == 0


def test_eksik_metrik_izni_ile_eklenebilir():
    """Kismi kayitlar (orn. yalnizca kimlik sutunlari) istege bagli kabul edilir."""
    partial = pd.DataFrame({
        'calisma': [db.STUDY_HOMOGENEOUS], 'deney_id': ['A_N3'],
        'kombinasyon_id': ['A'], 'tarih': ['2026-08-20'], 'faz': ['faz1'],
        'N': [3], 'P2': ['c'], 'P3': ['c'], 'P4': ['b'], 'P5': ['abc'],
        'P6': ['c'], 'tekrar': [10], 'notlar': [''],
    })
    for column in db.SCHEMA:
        if column not in partial.columns:
            partial[column] = pd.NA

    with pytest.raises(ValueError):
        db.append(db.empty_database(), partial.drop(columns=db.BASE_METRIC_COLUMNS))

    database = db.append(db.empty_database(), partial, require_metrics=False)
    assert len(database) == 1
