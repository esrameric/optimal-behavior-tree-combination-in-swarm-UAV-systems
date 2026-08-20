"""Faz 0 kurulum dogrulamasi: analiz tarafinin pytest altyapisi ve bagimliliklari."""

import swarm_bt_analysis


def test_paket_ice_aktarilabiliyor():
    """Analiz paketi colcon kurulumundan sonra ice aktarilabilmeli."""
    assert swarm_bt_analysis.__version__


def test_pandas_kullanilabilir():
    """Metrik tablolari pandas ile uretilecek; bagimlilik erkenden dogrulanir."""
    import pandas as pd

    frame = pd.DataFrame({'n': [3, 5], 'metrik': [1.0, 2.0]})
    assert list(frame['n']) == [3, 5]
    assert frame['metrik'].mean() == 1.5
