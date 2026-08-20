from setuptools import find_packages, setup

package_name = 'swarm_bt_analysis'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='emeric.topaktas',
    maintainer_email='mericesra01@gmail.com',
    description=('Deney ciktilarindan metrik hesaplama ve gorsellestirme; '
                 'rosbag2/CSV son-islemesi.'),
    license='Proprietary',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'ofat_report = swarm_bt_analysis.report_ofat:main',
        ],
    },
)
