r"""
Faz 1 - tek koşu: hafif kinematik simulatorde bir deney konfigurasyonu.

Kullanim:
    ros2 launch swarm_bt_bringup phase1_single_run.launch.py
    ros2 launch swarm_bt_bringup phase1_single_run.launch.py \\
        config:=experiment_P2b_P3c_P4c_P5bc_P6c_N5.yaml seed:=3 failure:=true
"""

import os

from ament_index_python.packages import get_package_prefix, get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, OpaqueFunction
from launch.substitutions import LaunchConfiguration


def _build_run(context, *args, **kwargs):
    """Secilen config ve seceneklerden simulator cagrisini kurar."""
    config_dir = os.path.join(
        get_package_share_directory('swarm_bt_bringup'), 'config')
    config_name = LaunchConfiguration('config').perform(context)
    config_path = os.path.join(config_dir, config_name)

    if not os.path.exists(config_path):
        raise RuntimeError(
            f'Deney config dosyasi bulunamadi: {config_path}\n'
            f'Mevcut dosyalar: {sorted(os.listdir(config_dir))}')

    arguments = ['--config', config_path,
                 '--seed', LaunchConfiguration('seed').perform(context)]
    if LaunchConfiguration('failure').perform(context).lower() == 'true':
        arguments.append('--failure')

    # launch_ros.Node yerine ExecuteProcess: bunlar ROS node'u degil, bagimsiz
    # calistirilabilirler. Node kullanilirsa launch komut satirina --ros-args
    # ekliyor ve arguman ayristirici bunu tanimiyor.
    executable = os.path.join(
        get_package_prefix('swarm_bt_sim'), 'lib', 'swarm_bt_sim', 'sim_runner')
    return [ExecuteProcess(cmd=[executable] + arguments, output='screen')]


def generate_launch_description():
    """Launch tanimini uretir."""
    return LaunchDescription([
        DeclareLaunchArgument(
            'config',
            default_value='experiment_P2c_P3c_P4b_P5abc_P6c_N3.yaml',
            description='swarm_bt_bringup/config altindaki deney dosyasi'),
        DeclareLaunchArgument(
            'seed', default_value='0',
            description='rastgelelik tohumu (tekrarlanabilirlik)'),
        DeclareLaunchArgument(
            'failure', default_value='false',
            description='Bolum 2.3 surpriz olayi: bir drone arizalandirilsin mi'),
        OpaqueFunction(function=_build_run),
    ])
