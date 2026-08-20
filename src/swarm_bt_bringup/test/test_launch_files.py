"""
Faz 1 launch dosyalarinin gecerliligini dogrular.

Launch dosyalari yalnizca calistirilinca hata verirse bu gec fark edilir;
burada import edilip LaunchDescription uretimleri ve refere ettikleri
calistirilabilirlerin varligi kontrol edilir.
"""

import importlib.util
import os

from ament_index_python.packages import get_package_prefix, get_package_share_directory

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument

import pytest

LAUNCH_FILES = ['phase1_single_run.launch.py', 'phase1_ofat_sweep.launch.py']

EXPECTED_EXECUTABLES = ['sim_runner', 'ofat_sweep', 'calibrate_rcomm', 'calibrate_threshold']


def _launch_dir():
    return os.path.join(get_package_share_directory('swarm_bt_bringup'), 'launch')


def _load(name):
    path = os.path.join(_launch_dir(), name)
    spec = importlib.util.spec_from_file_location(name.replace('.', '_'), path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


@pytest.mark.parametrize('name', LAUNCH_FILES)
def test_launch_dosyasi_mevcut(name):
    """Her launch dosyasi kurulmus olmali."""
    assert os.path.exists(os.path.join(_launch_dir(), name))


@pytest.mark.parametrize('name', LAUNCH_FILES)
def test_launch_tanimi_uretilebiliyor(name):
    """generate_launch_description() gecerli bir tanim dondurmeli."""
    module = _load(name)
    description = module.generate_launch_description()
    assert isinstance(description, LaunchDescription)
    assert description.entities


@pytest.mark.parametrize('name', LAUNCH_FILES)
def test_launch_argumanlari_aciklamali(name):
    """Her launch argumaninin varsayilani ve aciklamasi olmali."""
    module = _load(name)
    arguments = [
        entity for entity in module.generate_launch_description().entities
        if isinstance(entity, DeclareLaunchArgument)
    ]
    assert arguments, f'{name} hic arguman bildirmiyor'
    for argument in arguments:
        assert argument.default_value is not None, f'{name}: {argument.name} varsayilansiz'
        assert argument.description, f'{name}: {argument.name} aciklamasiz'


def test_tek_koşu_launch_dosyasi_beklenen_argumanlari_bildirir():
    """Tek koşu launch'i config, seed ve failure argumanlarini almali."""
    module = _load('phase1_single_run.launch.py')
    names = {
        entity.name for entity in module.generate_launch_description().entities
        if isinstance(entity, DeclareLaunchArgument)
    }
    assert {'config', 'seed', 'failure'} <= names


def test_tarama_launch_dosyasi_tekrar_argumani_bildirir():
    """OFAT tarama launch'i tekrar sayisini almali (plan: >= 10)."""
    module = _load('phase1_ofat_sweep.launch.py')
    arguments = {
        entity.name: entity.default_value[0].text
        for entity in module.generate_launch_description().entities
        if isinstance(entity, DeclareLaunchArgument)
    }
    assert 'repetitions' in arguments
    assert int(arguments['repetitions']) >= 10, 'plan kombinasyon basina >= 10 tekrar istiyor'


@pytest.mark.parametrize('executable', EXPECTED_EXECUTABLES)
def test_refere_edilen_calistirilabilirler_kurulu(executable):
    """Launch dosyalarinin cagirdigi araclar kurulum agacinda bulunmali."""
    path = os.path.join(
        get_package_prefix('swarm_bt_sim'), 'lib', 'swarm_bt_sim', executable)
    assert os.path.exists(path), f'{executable} kurulu degil: {path}'
    assert os.access(path, os.X_OK), f'{executable} calistirilabilir degil'


def test_varsayilan_config_dosyasi_mevcut():
    """Tek koşu launch'inin varsayilan deney dosyasi kurulmus olmali."""
    module = _load('phase1_single_run.launch.py')
    default = next(
        entity.default_value[0].text
        for entity in module.generate_launch_description().entities
        if isinstance(entity, DeclareLaunchArgument) and entity.name == 'config'
    )
    config_path = os.path.join(
        get_package_share_directory('swarm_bt_bringup'), 'config', default)
    assert os.path.exists(config_path), f'varsayilan config kurulu degil: {config_path}'
