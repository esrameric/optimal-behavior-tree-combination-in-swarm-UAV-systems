"""
Faz 2 Gazebo dunyasi ureteci.

N adet hiz-kontrollu gövde ve gorev alanini temsil eden bir zemin iceren SDF
uretir. Her gövde:
  - VelocityControl eklentisiyle /model/<ad>/cmd_vel topic'inden surulur
  - PosePublisher eklentisiyle konumunu yayinlar

NOT: Bu dunya PX4 SITL ucus yiginini KULLANMAZ. Gerekce ve sinirliliklar
repo README'sindeki Varsayimlar V18'de aciklanmistir.
"""

MODEL_TEMPLATE = """
    <model name="{name}">
      <pose>{x} {y} {z} 0 0 0</pose>
      <link name="body">
        <!-- Yercekimi kapali: bu calisma 2D sabit yukseklikte tarama modelliyor
             (plan: 3D yukseklik kapsam disi). Ucus dinamigi yerine hiz kontrolu
             kullanildigi icin gövdenin duşmemesi gerekiyor. -->
        <gravity>false</gravity>
        <inertial>
          <mass>1.5</mass>
          <inertia><ixx>0.03</ixx><iyy>0.03</iyy><izz>0.05</izz>
            <ixy>0</ixy><ixz>0</ixz><iyz>0</iyz></inertia>
        </inertial>
        <visual name="visual">
          <geometry><box><size>0.6 0.6 0.2</size></box></geometry>
          <material><ambient>{color} 1</ambient><diffuse>{color} 1</diffuse></material>
        </visual>
        <collision name="collision">
          <geometry><box><size>0.6 0.6 0.2</size></box></geometry>
        </collision>
      </link>

      <!-- /model/<ad>/cmd_vel ile hiz komutu -->
      <plugin filename="gz-sim-velocity-control-system"
              name="gz::sim::systems::VelocityControl">
        <topic>/model/{name}/cmd_vel</topic>
      </plugin>

      <!-- /model/<ad>/pose ile konum yayini -->
      <plugin filename="gz-sim-pose-publisher-system"
              name="gz::sim::systems::PosePublisher">
        <publish_link_pose>false</publish_link_pose>
        <publish_model_pose>true</publish_model_pose>
        <use_pose_vector_msg>false</use_pose_vector_msg>
        <update_frequency>50</update_frequency>
        <!-- Duran gövdeler icin de yayin sart: aksi halde kilitlenme olusuyor.
             Dugum ilk konumu bekliyor, Gazebo ise konum DEGISMEDIGI icin hic
             yayinlamiyor; hicbir hiz komutu gitmiyor ve hicbir sey kimildamiyor. -->
        <static_publisher>true</static_publisher>
        <static_update_frequency>10</static_update_frequency>
      </plugin>
    </model>
"""

WORLD_TEMPLATE = """<?xml version="1.0" ?>
<sdf version="1.9">
  <world name="{world_name}">
    <!-- Gercek zamanli kosmak ZORUNLU: suru dugumu duvar saatiyle tikliyor.
         Gazebo daha hizli koşarsa gövde bir dugum tick'inde hedefi ASAR,
         varis hic tetiklenmez ve gorev bitmez (olculdu). -->
    <physics name="realtime" type="ignored">
      <max_step_size>0.004</max_step_size>
      <real_time_factor>{real_time_factor}</real_time_factor>
    </physics>

    <plugin filename="gz-sim-physics-system" name="gz::sim::systems::Physics"/>
    <plugin filename="gz-sim-user-commands-system" name="gz::sim::systems::UserCommands"/>
    <plugin filename="gz-sim-scene-broadcaster-system"
            name="gz::sim::systems::SceneBroadcaster"/>

    <light type="directional" name="sun">
      <cast_shadows>false</cast_shadows>
      <pose>0 0 100 0 0 0</pose>
      <diffuse>0.8 0.8 0.8 1</diffuse>
      <direction>-0.5 0.1 -0.9</direction>
    </light>

    <!-- Gorev alani zemini: {area_side} x {area_side} m -->
    <model name="mission_area">
      <static>true</static>
      <link name="ground">
        <collision name="collision">
          <geometry><plane><normal>0 0 1</normal>
            <size>{area_side} {area_side}</size></plane></geometry>
        </collision>
        <visual name="visual">
          <geometry><plane><normal>0 0 1</normal>
            <size>{area_side} {area_side}</size></plane></geometry>
          <material><ambient>0.3 0.35 0.3 1</ambient>
            <diffuse>0.3 0.35 0.3 1</diffuse></material>
        </visual>
      </link>
    </model>
{models}
  </world>
</sdf>
"""

_COLORS = [
    '0.9 0.2 0.2', '0.2 0.6 0.9', '0.3 0.8 0.3',
    '0.9 0.7 0.2', '0.7 0.3 0.9', '0.2 0.8 0.8', '0.9 0.5 0.7',
]


def build_world(n_agents, area_side=400.0, altitude=10.0, model_prefix='drone',
                world_name='swarm_area', real_time_factor=1.0, positions=None):
    """
    N gövdeli Faz 2 dunyasinin SDF metnini uretir.

    positions verilirse gövdeler o konumlara yerlestirilir. Faz 2 launch'i
    bunu simulatorden sorar: iki fazin AYNI kalkis geometrisinden baslamasi,
    kod-seviyesi <-> Gazebo karsilastirmasinin on kosuludur.
    """
    if n_agents <= 0:
        raise ValueError('n_agents pozitif olmali')
    if positions is not None and len(positions) != n_agents:
        raise ValueError(
            f'{n_agents} konum bekleniyordu, {len(positions)} verildi')

    models = []
    spacing = area_side / (n_agents + 1)
    for index in range(n_agents):
        if positions is not None:
            x, y = positions[index]
        else:
            x, y = spacing * (index + 1), spacing
        models.append(MODEL_TEMPLATE.format(
            name=f'{model_prefix}_{index}',
            x=round(x, 3),
            y=round(y, 3),
            z=altitude,
            color=_COLORS[index % len(_COLORS)],
        ))

    return WORLD_TEMPLATE.format(
        world_name=world_name,
        area_side=area_side,
        real_time_factor=real_time_factor,
        models=''.join(models),
    )


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Faz 2 Gazebo dunyasi uretir.')
    parser.add_argument('-n', '--n-agents', type=int, default=3)
    parser.add_argument('-a', '--area-side', type=float, default=400.0)
    parser.add_argument('-o', '--output', help='cikti SDF dosyasi; yoksa stdout')
    args = parser.parse_args()

    world = build_world(args.n_agents, args.area_side)
    if args.output:
        with open(args.output, 'w', encoding='utf-8') as handle:
            handle.write(world)
        print(f'dunya yazildi: {args.output}')
    else:
        print(world)
