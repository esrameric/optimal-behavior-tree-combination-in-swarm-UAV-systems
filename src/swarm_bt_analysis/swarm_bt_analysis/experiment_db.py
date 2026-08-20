"""
Ortak deney veritabani - plan Bolum 8.

Plan Bolum 8 su sablonu veriyor:

    deney_id, tarih, faz, N, P2, P3, P4, P5, P6,
    gorev_tamamlama_suresi, atama_kararliligi, churn_orani,
    kapsama_dengesizligi, karsilasma_sikligi, iletisim_yuku,
    tick_maliyeti, carpisma_sayisi, notlar

ve sunu istiyor: "Bu sablonu yapilacaklar.md'deki deney veritabanina yeni
sutunlarla (N, atama_kararliligi, churn_orani, kapsama_dengesizligi,
karsilasma_sikligi) genislet -- TEK BIR ORTAK VERITABANINDA tutmak, iki
calisma arasi karsilastirmayi kolaylastirir."

Bu modul o ortak veritabanini tanimlar, dogrular ve tarama ciktilarini ona
ekler. Iki calismanin satirlari ``calisma`` sutunuyla ayrilir.
"""

from __future__ import annotations

import argparse
import datetime as dt
import os
import sys

import pandas as pd

#: Calisma etiketleri; iki calismanin satirlari bununla ayrilir.
STUDY_HOMOGENEOUS = 'homojen'
STUDY_HETEROGENEOUS = 'heterojen'

#: Kimlik sutunlari (plan Bolum 8 sablonu + calisma ayrimi).
IDENTITY_COLUMNS = [
    'calisma',      # homojen / heterojen -- ORTAK veritabaninin ayirt edicisi
    'deney_id',
    'kombinasyon_id',
    'tarih',
    'faz',
    'N',            # <- bu calismanin ekledigi olcek sutunu
    'P2', 'P3', 'P4', 'P5', 'P6',
    'tekrar',
]

#: Plan Bolum 6'daki temel metrikler.
BASE_METRIC_COLUMNS = [
    'gorev_tamamlama_suresi',
    'iletisim_yuku',
    'tick_maliyeti',
    'carpisma_sayisi',
]

#: Bu calismaya ozgu, plan Bolum 8'in eklenmesini istedigi metrikler.
SCALE_METRIC_COLUMNS = [
    'atama_kararliligi',
    'churn_orani',
    'kapsama_dengesizligi',
    'karsilasma_sikligi',
]

#: Serbest not sutunu, sablonun sonunda.
TRAILING_COLUMNS = ['notlar']

#: Veritabaninin tam sutun sirasi.
SCHEMA = IDENTITY_COLUMNS + BASE_METRIC_COLUMNS + SCALE_METRIC_COLUMNS + TRAILING_COLUMNS

#: Farkli araclarin sutun adlari -> sema adlari.
#:
#: Faz 2 kampanya ciktisi (run_phase2.sh) kisa adlar kullaniyor; ortak
#: veritabaninda iki fazin satirlari AYNI sutun kumesinde bulusmali.
COLUMN_ALIASES = {
    'gorev_suresi': 'gorev_tamamlama_suresi',
    'tick': 'tick_maliyeti',
    'karsilasma': 'karsilasma_sikligi',
    'carpisma': 'carpisma_sayisi',
}


def empty_database():
    """Sema ile uyumlu bos bir veritabani dondurur."""
    return pd.DataFrame(columns=SCHEMA)


def validate(frame, require_metrics=True):
    """
    Tabloyu semaya gore dogrular; sorunlari liste olarak dondurur.

    Bos liste, tablonun ortak veritabanina eklenebilir oldugu anlamina gelir.
    """
    issues = []

    missing = [column for column in IDENTITY_COLUMNS if column not in frame.columns]
    if missing:
        issues.append(f'eksik kimlik sutunlari: {missing}')

    if require_metrics:
        missing_metrics = [
            column for column in BASE_METRIC_COLUMNS + SCALE_METRIC_COLUMNS
            if column not in frame.columns
        ]
        if missing_metrics:
            issues.append(f'eksik metrik sutunlari: {missing_metrics}')

    if 'calisma' in frame.columns:
        unknown = set(frame['calisma'].dropna()) - {STUDY_HOMOGENEOUS, STUDY_HETEROGENEOUS}
        if unknown:
            issues.append(f'bilinmeyen calisma etiketi: {sorted(unknown)}')

    if 'N' in frame.columns and not frame['N'].dropna().empty:
        if (pd.to_numeric(frame['N'], errors='coerce').dropna() <= 0).any():
            issues.append('N pozitif olmali')

    key = ['calisma', 'deney_id', 'faz']
    if all(column in frame.columns for column in key):
        duplicated = frame.duplicated(subset=key).sum()
        if duplicated:
            issues.append(f'{duplicated} satir (calisma, deney_id, faz) anahtarinda tekrarli')

    return issues


def from_sweep(sweep, study=STUDY_HOMOGENEOUS, date=None, notes=''):
    """
    ofat_sweep ciktisini ortak veritabani semasina cevirir.

    Tarama CSV'sinde olmayan sema sutunlari bos birakilir; fazladan sutunlar
    duşurulur, boylece iki calisma ayni sutun kumesinde bulusur.
    """
    frame = sweep.copy()
    frame = frame.rename(columns={
        source: target for source, target in COLUMN_ALIASES.items()
        if source in frame.columns and target not in frame.columns
    })
    frame['calisma'] = study
    frame['tarih'] = date or dt.date.today().isoformat()
    if 'notlar' not in frame.columns:
        frame['notlar'] = notes
    else:
        frame['notlar'] = frame['notlar'].fillna(notes)

    for column in SCHEMA:
        if column not in frame.columns:
            frame[column] = pd.NA

    return frame[SCHEMA]


def append(database, rows, require_metrics=True):
    """
    Satirlari veritabanina ekler; ayni anahtardaki eski satirlari degistirir.

    Anahtar (calisma, deney_id, faz). Bir taramanin tekrar koşulmasi
    veritabanini sisirmemeli.
    """
    issues = validate(rows, require_metrics=require_metrics)
    if issues:
        raise ValueError('sema uyumsuzlugu: ' + '; '.join(issues))

    if database is None or database.empty:
        return rows[SCHEMA].reset_index(drop=True)

    key = ['calisma', 'deney_id', 'faz']
    # Tamamen bos sutunlar concat'te dtype uyarisi uretiyor; birlestirmeden
    # once duşurulup sonra geri eklenirler.
    frames = [
        frame[SCHEMA].dropna(axis=1, how='all')
        for frame in (database, rows) if not frame.empty
    ]
    merged = pd.concat(frames, ignore_index=True)
    for column in SCHEMA:
        if column not in merged.columns:
            merged[column] = pd.NA
    merged = merged[SCHEMA].drop_duplicates(subset=key, keep='last')
    return merged.reset_index(drop=True)


def load(path):
    """Veritabanini okur; dosya yoksa bos bir tablo dondurur."""
    if not os.path.exists(path):
        return empty_database()
    return pd.read_csv(path)


def save(database, path):
    """Veritabanini sema sutun sirasiyla yazar."""
    database[SCHEMA].to_csv(path, index=False)
    return path


def summary(database):
    """Veritabaninin calisma/faz/olcek dagilimini ozetler."""
    if database.empty:
        return pd.DataFrame(columns=['calisma', 'faz', 'N', 'satir'])
    return (
        database.groupby(['calisma', 'faz', 'N'])
        .size().reset_index(name='satir')
        .sort_values(['calisma', 'faz', 'N'])
    )


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(
        description='Tarama ciktisini ortak deney veritabanina ekler (plan Bolum 8).')
    parser.add_argument('sweep_csv', help='ofat_sweep ciktisi')
    parser.add_argument('-d', '--database', default='experiments/deney_veritabani.csv',
                        help='ortak veritabani dosyasi')
    parser.add_argument('-s', '--study', default=STUDY_HOMOGENEOUS,
                        choices=[STUDY_HOMOGENEOUS, STUDY_HETEROGENEOUS],
                        help='satirlarin ait oldugu calisma')
    parser.add_argument('-n', '--notes', default='', help='satirlara yazilacak not')
    parser.add_argument('--allow-missing-metrics', action='store_true',
                        help='eksik metrik sutunlarina izin ver (kismi kayitlar icin)')
    args = parser.parse_args(argv)

    from swarm_bt_analysis import ofat
    rows = from_sweep(ofat.load_sweep(args.sweep_csv), study=args.study, notes=args.notes)
    if args.allow_missing_metrics:
        issues = validate(rows, require_metrics=False)
        if issues:
            raise SystemExit('sema uyumsuzlugu: ' + '; '.join(issues))
    database = append(
        load(args.database), rows, require_metrics=not args.allow_missing_metrics)
    save(database, args.database)

    print(f'{len(rows)} satir eklendi -> {args.database} (toplam {len(database)})')
    summary(database).to_csv(sys.stdout, index=False)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
