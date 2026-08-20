"""rosbag2 olay kaydindan metrik hesabinin birim testleri (plan Bolum 6)."""

import pytest

from swarm_bt_analysis import bag_metrics as bm


def test_sebep_adlari_msg_dosyasiyla_ayni():
    assert bm.reason_name(0) == 'alan_takasi'
    assert bm.reason_name(1) == 'ariza_devralma'
    assert bm.reason_name(2) == 'ortak_tarama'
    assert bm.reason_name(3) == 'baslangic_atamasi'
    assert bm.reason_name(4) == 'ariza'
    assert bm.reason_name(9).startswith('bilinmeyen')


def test_olaysiz_kayit_sifir_metrik_verir():
    metrics = bm.compute_metrics([], [], agent_count=3)
    assert metrics['atama_olayi'] == 0
    assert metrics['atama_kararliligi'] == 0.0
    assert metrics['churn_orani'] == 0.0


def test_gecersiz_ajan_sayisi_reddedilir():
    with pytest.raises(ValueError, match='pozitif olmali'):
        bm.compute_metrics([], [], agent_count=0)


def test_atama_kararliligi_ajan_basina_hesaplanir():
    events = [{'agent_id': i % 3, 'reason': 0, 'peer_id': -1, 'cells_changed': 2}
              for i in range(6)]
    metrics = bm.compute_metrics(events, [], agent_count=3)
    assert metrics['atama_kararliligi'] == pytest.approx(2.0)


def test_churn_orani_karsilasma_basina_hesaplanir():
    """Bir takas IKI olay uretir; churn karsilasma basina sayilir."""
    assignments = [
        {'agent_id': 0, 'reason': 0, 'peer_id': 1, 'cells_changed': 5},
        {'agent_id': 1, 'reason': 0, 'peer_id': 0, 'cells_changed': 5},
    ]
    encounters = [{'agent_a': 0, 'agent_b': 1}, {'agent_a': 0, 'agent_b': 2}]
    metrics = bm.compute_metrics(assignments, encounters, agent_count=3)

    assert metrics['karsilasma'] == 2
    assert metrics['churn_olayi'] == pytest.approx(1.0)
    assert metrics['churn_orani'] == pytest.approx(0.5)


def test_ortaksiz_olaylar_churn_a_girmez():
    """Bosta devralma bir karsilasma sonucu degildir; churn payina girmemeli."""
    assignments = [
        {'agent_id': 0, 'reason': 1, 'peer_id': -1, 'cells_changed': 20},
        {'agent_id': 1, 'reason': 4, 'peer_id': -1, 'cells_changed': 20},
    ]
    encounters = [{'agent_a': 0, 'agent_b': 1}]
    metrics = bm.compute_metrics(assignments, encounters, agent_count=3)
    assert metrics['churn_olayi'] == pytest.approx(0.0)
    assert metrics['churn_orani'] == pytest.approx(0.0)
    assert metrics['atama_olayi'] == 2


def test_sebep_dagilimi_sayilir():
    assignments = [
        {'agent_id': 0, 'reason': 0, 'peer_id': 1, 'cells_changed': 5},
        {'agent_id': 1, 'reason': 0, 'peer_id': 0, 'cells_changed': 5},
        {'agent_id': 2, 'reason': 4, 'peer_id': -1, 'cells_changed': 12},
    ]
    metrics = bm.compute_metrics(assignments, [], agent_count=3)
    assert metrics['sebep_dagilimi'] == {'alan_takasi': 2, 'ariza': 1}


def test_devredilen_hucre_toplanir():
    assignments = [
        {'agent_id': 0, 'reason': 0, 'peer_id': 1, 'cells_changed': 5},
        {'agent_id': 1, 'reason': 1, 'peer_id': -1, 'cells_changed': 7},
    ]
    metrics = bm.compute_metrics(assignments, [], agent_count=2)
    assert metrics['devredilen_hucre'] == 12


def test_eksik_alanlar_varsayilanla_islenir():
    """Kayittan gelen sozlukte alan eksikse hesap cokmemeli."""
    metrics = bm.compute_metrics([{'agent_id': 0}], [], agent_count=1)
    assert metrics['atama_olayi'] == 1
    assert metrics['devredilen_hucre'] == 0
