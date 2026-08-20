"""
rosbag2 olay kaydindan Bolum 6 metriklerini hesaplar.

Plan Bolum 6: "Atama kararliligi ve churn orani icin otomatik loglama ekle
(her swap olayinda rosbag2'ye event yaz)". Bu modul dongunun diger ucudur:
kaydedilen olaylardan metrikleri geri hesaplar.

Metriklerin KAYITTAN turetilebilir olmasi bir dogrulama araci da saglar --
koşu sirasinda hesaplanan degerlerle kayittan hesaplananlar birbirini
tutmuyorsa loglamada bir eksik var demektir.

Tasarim: metrik hesabi saf sozluk listeleri uzerinde calisir (test edilebilir),
rosbag2 okumasi ince bir adaptordur.
"""

from __future__ import annotations

import argparse
import json
import sys

#: AssignmentChange.reason alaninin anlamlari (msg dosyasiyla ayni sirada).
REASONS = {
    0: 'alan_takasi',
    1: 'ariza_devralma',
    2: 'ortak_tarama',
    3: 'baslangic_atamasi',
    4: 'ariza',
}

#: Kayittan okunan topic adlari.
ASSIGNMENT_TOPIC = '/swarm/assignment_change'
ENCOUNTER_TOPIC = '/swarm/encounter'
STATUS_TOPIC = '/swarm/agent_status'


def reason_name(code):
    """Sebep kodunu okunabilir ada cevirir."""
    return REASONS.get(int(code), f'bilinmeyen_{code}')


def compute_metrics(assignment_events, encounter_events, agent_count):
    """
    Olay listelerinden Bolum 6 metriklerini hesaplar.

    assignment_events: {'agent_id', 'reason', 'peer_id', 'cells_changed'} sozlukleri
    encounter_events : {'agent_a', 'agent_b'} sozlukleri
    """
    if agent_count <= 0:
        raise ValueError('agent_count pozitif olmali')

    encounters = len(encounter_events)
    # Churn orani: gercek degisiklige yol acan karsilasma / toplam karsilasma.
    # Bir takas iki ajan icin iki olay uretir; karsilasma basina saymak icin
    # ORTAKLI olaylar ciftlenir.
    encounter_driven = [
        event for event in assignment_events
        if int(event.get('peer_id', -1)) >= 0
    ]
    churn_events = len(encounter_driven) / 2.0

    by_reason = {}
    for event in assignment_events:
        name = reason_name(event.get('reason', 3))
        by_reason[name] = by_reason.get(name, 0) + 1

    return {
        'ajan_sayisi': agent_count,
        'atama_olayi': len(assignment_events),
        'atama_kararliligi': len(assignment_events) / agent_count,
        'karsilasma': encounters,
        'churn_olayi': churn_events,
        'churn_orani': (churn_events / encounters) if encounters else 0.0,
        'devredilen_hucre': sum(
            int(event.get('cells_changed', 0)) for event in assignment_events),
        'sebep_dagilimi': by_reason,
    }


def read_bag(path, storage_id='sqlite3'):
    """
    rosbag2 kaydindan olaylari okur.

    ROS2 calisma zamani gerektirir; metrik hesabi (compute_metrics) bundan
    bagimsizdir ve saf Python ile test edilebilir.
    """
    import rosbag2_py
    from rclpy.serialization import deserialize_message
    from rosidl_runtime_py.utilities import get_message

    reader = rosbag2_py.SequentialReader()
    reader.open(
        rosbag2_py.StorageOptions(uri=path, storage_id=storage_id),
        rosbag2_py.ConverterOptions('', ''))
    types = {topic.name: topic.type for topic in reader.get_all_topics_and_types()}

    assignments = []
    encounters = []
    agents = set()

    while reader.has_next():
        topic, data, _ = reader.read_next()
        if topic not in types:
            continue
        if topic == ASSIGNMENT_TOPIC:
            message = deserialize_message(data, get_message(types[topic]))
            assignments.append({
                'agent_id': message.agent_id,
                'reason': message.reason,
                'peer_id': message.peer_id,
                'cells_changed': message.cells_changed,
                'change_index': message.change_index,
            })
            agents.add(message.agent_id)
        elif topic == ENCOUNTER_TOPIC:
            message = deserialize_message(data, get_message(types[topic]))
            encounters.append({
                'agent_a': message.agent_a,
                'agent_b': message.agent_b,
                'distance': message.distance,
            })
            agents.update((message.agent_a, message.agent_b))
        elif topic == STATUS_TOPIC:
            message = deserialize_message(data, get_message(types[topic]))
            agents.add(message.agent_id)

    return assignments, encounters, len(agents)


def main(argv=None):
    """Komut satiri giris noktasi."""
    parser = argparse.ArgumentParser(
        description='rosbag2 kaydindan Bolum 6 metriklerini hesaplar.')
    parser.add_argument('bag', help='rosbag2 dizini')
    parser.add_argument('-n', '--agent-count', type=int, default=0,
                        help='ajan sayisi; verilmezse kayittan cikarilir')
    args = parser.parse_args(argv)

    assignments, encounters, detected = read_bag(args.bag)
    agent_count = args.agent_count or detected
    metrics = compute_metrics(assignments, encounters, agent_count)
    json.dump(metrics, sys.stdout, indent=2, ensure_ascii=False)
    sys.stdout.write('\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
