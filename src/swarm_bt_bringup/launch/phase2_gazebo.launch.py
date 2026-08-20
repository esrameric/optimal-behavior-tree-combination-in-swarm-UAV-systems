r"""
Faz 2 - Gazebo dogrulamasi: Gazebo Harmonic + ros_gz koprusu + suru dugumu.

Kullanim:
    ros2 launch swarm_bt_bringup phase2_gazebo.launch.py n_agents:=3
    ros2 launch swarm_bt_bringup phase2_gazebo.launch.py \\
        n_agents:=5 seed:=2 record:=true gui:=false

Dunya, istenen drone sayisina gore calisma aninda uretilir
(worlds/generate_world.py).
"""

import importlib.util
import os
import tempfile

import subprocess

from ament_index_python.packages import get_package_prefix, get_package_share_directory

from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument, EmitEvent, ExecuteProcess, OpaqueFunction, RegisterEventHandler)
from launch.event_handlers import OnProcessExit
from launch.events import Shutdown
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def _load_world_builder():
    """worlds/generate_world.py modulunu yukler."""
    path = os.path.join(
        get_package_share_directory('swarm_bt_bringup'), 'worlds', 'generate_world.py')
    spec = importlib.util.spec_from_file_location('generate_world', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _query_launch_positions(config_path, seed, n_agents):
    """Simulatorden ajanlarin kalkis konumlarini sorar."""
    executable = os.path.join(
        get_package_prefix('swarm_bt_sim'), 'lib', 'swarm_bt_sim', 'sim_runner')
    result = subprocess.run(
        [executable, '--config', config_path, '--seed', str(seed),
         '--n', str(n_agents), '--print-launch-positions'],
        capture_output=True, text=True, check=True)
    positions = []
    for line in result.stdout.strip().splitlines():
        x, y = line.split()
        positions.append((float(x), float(y)))
    if len(positions) != n_agents:
        raise RuntimeError(
            f'{n_agents} kalkis konumu bekleniyordu, {len(positions)} geldi')
    return positions


def _setup(context, *args, **kwargs):
    n_agents = int(LaunchConfiguration('n_agents').perform(context))
    seed = LaunchConfiguration('seed').perform(context)
    gui = LaunchConfiguration('gui').perform(context).lower() == 'true'
    record = LaunchConfiguration('record').perform(context).lower() == 'true'
    config_name = LaunchConfiguration('config').perform(context)

    config_path = os.path.join(
        get_package_share_directory('swarm_bt_bringup'), 'config', config_name)
    if not os.path.exists(config_path):
        raise RuntimeError(f'Deney config dosyasi bulunamadi: {config_path}')

    # Kalkis konumlari Faz 1 ile AYNI olmali; aksi halde iki fazin
    # karsilastirmasi kalkis geometrisi farkini olcerdi. Konumlar simulatorun
    # kendisinden sorulur (ayni tohum, ayni config).
    launch_positions = _query_launch_positions(config_path, seed, n_agents)

    builder = _load_world_builder()
    world_path = os.path.join(
        tempfile.gettempdir(), f'swarm_bt_phase2_n{n_agents}_seed{seed}.sdf')
    with open(world_path, 'w', encoding='utf-8') as handle:
        handle.write(builder.build_world(n_agents, positions=launch_positions))

    actions = []

    # Gazebo sunucusu; gui=false ise bassiz (-s).
    gz_args = ['gz', 'sim', '-r', '-v', '1']
    if not gui:
        gz_args.append('-s')
    gz_args.append(world_path)
    actions.append(ExecuteProcess(cmd=gz_args, output='screen'))

    # ros_gz koprusu: her drone icin pose (gz -> ros), cmd_vel (ros -> gz).
    bridge_args = []
    for index in range(n_agents):
        name = f'drone_{index}'
        # Gazebo, HAREKET EDEN gövdelerin pozunu /pose'a, duranlarinkini
        # /pose_static'e yayinliyor. Ikisi de gerekli: aksi halde hicbir sey
        # kimildamadigi icin hic poz gelmiyor ve dugum ilk konumu bekleyerek
        # kilitleniyor.
        bridge_args.append(f'/model/{name}/pose@geometry_msgs/msg/Pose[gz.msgs.Pose')
        bridge_args.append(f'/model/{name}/pose_static@geometry_msgs/msg/Pose[gz.msgs.Pose')
        bridge_args.append(f'/model/{name}/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist')
    actions.append(Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        name='swarm_bt_gz_bridge',
        arguments=bridge_args,
        output='screen',
    ))

    # Suru koordinasyon dugumu: Faz 1 ile AYNI BT kodu, farkli pozisyon kaynagi.
    swarm_node = Node(
        package='swarm_bt_sim',
        executable='gazebo_swarm_node',
        name='swarm_bt_gazebo',
        output='screen',
        parameters=[{
            'config': config_path,
            'n_agents': n_agents,
            'seed': int(seed),
            'model_prefix': 'drone',
        }],
    )
    actions.append(swarm_node)
    # Koşu bitince Gazebo ve kopru de kapansin; aksi halde launch asili kalir.
    actions.append(RegisterEventHandler(OnProcessExit(
        target_action=swarm_node,
        on_exit=[EmitEvent(event=Shutdown(reason='koşu tamamlandi'))])))

    # Bolum 6: olay kayitlari rosbag2'ye yazilir.
    if record:
        bag_path = os.path.join(os.getcwd(), f'rosbag2_phase2_n{n_agents}_seed{seed}')
        actions.append(ExecuteProcess(
            cmd=['ros2', 'bag', 'record', '-o', bag_path,
                 '/swarm/encounter', '/swarm/agent_status', '/swarm/assignment_change'],
            output='screen',
        ))

    return actions


def generate_launch_description():
    """Launch tanimini uretir."""
    return LaunchDescription([
        DeclareLaunchArgument(
            'n_agents', default_value='3',
            description='drone sayisi (plan: N in {3, 5})'),
        DeclareLaunchArgument(
            'seed', default_value='0',
            description='rastgelelik tohumu (tekrarlanabilirlik)'),
        DeclareLaunchArgument(
            'config', default_value='experiment_P2c_P3c_P4b_P5abc_P6c_N3.yaml',
            description='swarm_bt_bringup/config altindaki deney dosyasi'),
        DeclareLaunchArgument(
            'gui', default_value='false',
            description='Gazebo arayuzu acilsin mi (bassiz koşu icin false)'),
        DeclareLaunchArgument(
            'record', default_value='false',
            description='olay kayitlari rosbag2 ile kaydedilsin mi (Bolum 6)'),
        OpaqueFunction(function=_setup),
    ])
