"""'Manipulasyona aciklik' hipotez testinin birim testleri (plan Bolum 5/Faz 3)."""

import io

import pytest

from swarm_bt_analysis import ofat, stability_hypothesis as sh


def _sweep(stability_n5='0.5', churn_n5='0.6'):
    """
    B kombinasyonu kararliligi artan, X kombinasyonu azalan bir tablo.

    Varsayilanda B daha kararli (1.0 -> 0.5), X daha oynak (0.4 -> 0.9).
    """
    return ofat.load_sweep(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,'
        'gorev_tamamlama_suresi,atama_kararliligi,churn_orani,kapsama_dengesizligi,'
        'karsilasma_sikligi,iletisim_yuku,tick_maliyeti,carpisma_sayisi\n'
        'B_N3,B,faz1,3,c,c,b,abc,c,10,100,1.0,0.5,2.0,10,20,1000,1\n'
        f'B_N5,B,faz1,5,c,c,b,abc,c,10,50,{stability_n5},{churn_n5},1.0,20,40,500,2\n'
        'X_N3,X,faz1,3,c,c,c,abc,c,10,90,0.4,0.3,2.0,10,20,900,1\n'
        'X_N5,X,faz1,5,c,c,c,abc,c,10,60,0.9,0.7,1.0,20,40,600,2\n'
    ))


def test_siniflandirma_yonu_dogru():
    """Sayac duşuyorsa daha kararli, yukseliyorsa daha oynak."""
    assert sh.classify(-0.2) == sh.MORE_STABLE
    assert sh.classify(+0.2) == sh.MORE_VOLATILE
    assert sh.classify(0.0) == sh.UNCHANGED


def test_degerlendirme_her_kombinasyon_icin_sonuc_uretir():
    evaluation = sh.evaluate(_sweep())
    results = dict(zip(evaluation['kombinasyon_id'], evaluation['sonuc']))
    assert results['B'] == sh.MORE_STABLE     # 1.0 -> 0.5
    assert results['X'] == sh.MORE_VOLATILE   # 0.4 -> 0.9


def test_bagil_degisim_hesaplanir():
    evaluation = sh.evaluate(_sweep()).set_index('kombinasyon_id')
    assert evaluation.loc['B', 'bagil_degisim'] == pytest.approx(-0.5)
    assert evaluation.loc['X', 'bagil_degisim'] == pytest.approx(1.25)


def test_karma_sonuc_oybirligi_saymaz():
    summary = sh.verdict(_sweep())
    assert not summary['oybirligi_mi']
    assert summary['kombinasyon_sayisi'] == 2
    assert set(summary['sayimlar']) == {sh.MORE_STABLE, sh.MORE_VOLATILE}


def test_oybirlikli_sonuc_isaretlenir():
    # X'i de kararli yap: 0.4 -> 0.9 yerine tabloyu B ile ayni yone cevir.
    frame = ofat.load_sweep(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,'
        'gorev_tamamlama_suresi,atama_kararliligi,churn_orani\n'
        'B_N3,B,faz1,3,c,c,b,abc,c,10,100,1.0,0.5\n'
        'B_N5,B,faz1,5,c,c,b,abc,c,10,50,0.5,0.3\n'
        'X_N3,X,faz1,3,c,c,c,abc,c,10,90,0.8,0.4\n'
        'X_N5,X,faz1,5,c,c,c,abc,c,10,60,0.6,0.2\n'
    ))
    summary = sh.verdict(frame)
    assert summary['oybirligi_mi']
    assert summary['sonuc'] == sh.MORE_STABLE
    assert summary['ortalama_delta'] == pytest.approx(-0.35)


def test_eksik_metrik_hata_verir():
    frame = ofat.load_sweep(io.StringIO(
        'deney_id,kombinasyon_id,faz,N,P2,P3,P4,P5,P6,tekrar,gorev_tamamlama_suresi\n'
        'B_N3,B,faz1,3,c,c,b,abc,c,10,100\n'
        'B_N5,B,faz1,5,c,c,b,abc,c,10,50\n'
    ))
    with pytest.raises(ValueError, match='sutununu icermiyor'):
        sh.evaluate(frame)


def test_rapor_metrigin_yonunu_acikliyor():
    """Metrik adi olculen seyin tersini cagristirdigi icin bu sart."""
    report = sh.build_report(_sweep())
    assert 'yükseldikçe sistem daha oynak' in report


def test_rapor_zit_yonlu_olcumleri_ayrica_isaretler():
    """Ajan basina ve karsilasma basina olcumler zit yone gidebilir."""
    # B: kararlilik duşuyor (daha kararli) ama churn yukseliyor (daha oynak)
    report = sh.build_report(_sweep(stability_n5='0.5', churn_n5='0.9'))
    assert 'ZIT yönde' in report
    assert 'ölçüm merceğine bağlı' in report


def test_rapor_ayni_yonlu_olcumde_uyari_vermez():
    report = sh.build_report(_sweep(stability_n5='1.5', churn_n5='0.9'))
    assert 'ZIT yönde' not in report
    assert 'aynı yönde' in report
