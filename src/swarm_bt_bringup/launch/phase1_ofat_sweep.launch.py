"""
Faz 1 - OFAT taramasi: her kombinasyon hem N=3 hem N=5 ile koşulur.

Cikti CSV olarak stdout'a yazilir; dosyaya yonlendirmek icin:
    ros2 launch swarm_bt_bringup phase1_ofat_sweep.launch.py > tarama.csv

Kullanim:
    ros2 launch swarm_bt_bringup phase1_ofat_sweep.launch.py repetitions:=10
"""

import os

from ament_index_python.packages import get_package_prefix

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    """Launch tanimini uretir."""
    executable = os.path.join(
        get_package_prefix('swarm_bt_sim'), 'lib', 'swarm_bt_sim', 'ofat_sweep')
    return LaunchDescription([
        DeclareLaunchArgument(
            'repetitions', default_value='10',
            description='kombinasyon x olcek basina tekrar sayisi (plan: >= 10)'),
        DeclareLaunchArgument(
            'phase', default_value='faz1',
            description='CSV faz sutununa yazilacak etiket'),
        # ROS node'u degil, bagimsiz calistirilabilir (bkz. phase1_single_run).
        ExecuteProcess(
            cmd=[
                executable,
                '--repetitions', LaunchConfiguration('repetitions'),
                '--phase', LaunchConfiguration('phase'),
            ],
            output='screen',
        ),
    ])
