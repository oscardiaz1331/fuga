"""Tests de map_iou.py con mapas sinteticos (sin ROS)."""
import importlib.util
import math
import os

import numpy as np
import pytest

# scripts/ no es un paquete Python: se carga el fichero directamente
_spec = importlib.util.spec_from_file_location('map_iou', os.path.join(
    os.path.dirname(__file__), '..', 'scripts', 'map_iou.py'))
map_iou = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(map_iou)

FREE, OCC, UNK = map_iou.FREE, map_iou.OCCUPIED, map_iou.UNKNOWN


def truth_grid():
    """Mapa verdad de 6 x 4 celdas de 1 m con origen (0, 0) y una L de muro."""
    state = np.full((4, 6), FREE, dtype=np.int8)
    state[0, :] = OCC      # fila de arriba (y en [3, 4])
    state[:, 0] = OCC      # columna izquierda (x en [0, 1])
    state[2, 3] = OCC      # muro suelto, rompe la simetria
    return map_iou.GridMap(state, 1.0, (0.0, 0.0, 0.0))


def in_map_frame(truth, start):
    """El mismo mapa visto desde el frame map de SLAM, que nace en start."""
    ox, oy, _ = truth.origin
    return map_iou.GridMap(truth.state.copy(), truth.resolution,
                           (ox - start[0], oy - start[1], 0.0))


def test_mismo_mapa_alineado_con_el_inicio_da_iou_1():
    truth = truth_grid()
    start = (2.0, 1.0, 0.0)
    result = map_iou.compare(in_map_frame(truth, start), truth, start)
    assert result.iou == pytest.approx(1.0)
    assert result.precision == pytest.approx(1.0)
    assert result.recall == pytest.approx(1.0)


def test_sin_alinear_con_el_inicio_no_coincide():
    truth = truth_grid()
    start = (2.0, 1.0, 0.0)
    result = map_iou.compare(in_map_frame(truth, start), truth, (0.0, 0.0, 0.0))
    assert result.iou < 0.5


def test_inicio_girado_90_grados():
    truth = truth_grid()
    sx, sy = 2.0, 1.0
    # Con yaw = +90 el mapa SLAM es la verdad girada -90 (sentido horario).
    # x_map = y - sy, y_map = -(x - sx): x_map en [-sy, 4 - sy], y_map en [sx - 6, sx]
    slam = map_iou.GridMap(np.rot90(truth.state, -1).copy(), 1.0,
                           (-sy, sx - 6.0, 0.0))
    result = map_iou.compare(slam, truth, (sx, sy, math.pi / 2))
    assert result.iou == pytest.approx(1.0)


def test_mapa_slam_vacio_da_iou_0():
    truth = truth_grid()
    slam = map_iou.GridMap(np.full((4, 6), UNK, dtype=np.int8), 1.0,
                           (0.0, 0.0, 0.0))
    result = map_iou.compare(slam, truth, (0.0, 0.0, 0.0))
    assert result.iou == 0.0
    assert result.explored_cells == 0


def test_muro_que_no_existe_baja_la_precision():
    truth = truth_grid()
    state = truth.state.copy()
    state[3, 5] = OCC  # celda libre en la verdad
    slam = map_iou.GridMap(state, 1.0, (0.0, 0.0, 0.0))
    result = map_iou.compare(slam, truth, (0.0, 0.0, 0.0))
    assert result.recall == pytest.approx(1.0)
    assert result.precision < 1.0
    assert result.false_positives == 1


def test_muros_fuera_del_mapa_verdad_cuentan_en_la_union():
    truth = truth_grid()
    # SLAM mas grande que la verdad, con un muro a la derecha del laberinto
    state = np.full((4, 8), FREE, dtype=np.int8)
    state[:, :6] = truth.state
    state[1, 7] = OCC
    slam = map_iou.GridMap(state, 1.0, (0.0, 0.0, 0.0))
    result = map_iou.compare(slam, truth, (0.0, 0.0, 0.0))
    assert result.false_positives == 1
    assert result.iou < 1.0


def test_zona_explorada_ignora_lo_que_slam_no_ha_visto():
    truth = truth_grid()
    state = truth.state.copy()
    state[:, 3:] = UNK  # SLAM solo ha visto la mitad izquierda
    slam = map_iou.GridMap(state, 1.0, (0.0, 0.0, 0.0))
    result = map_iou.compare(slam, truth, (0.0, 0.0, 0.0))
    assert result.iou < 1.0
    assert result.explored_iou == pytest.approx(1.0)


def test_tolerancia_absorbe_un_desfase_de_una_celda():
    truth = truth_grid()
    shifted = map_iou.GridMap(truth.state.copy(), 1.0, (1.0, 0.0, 0.0))
    exact = map_iou.compare(shifted, truth, (0.0, 0.0, 0.0))
    tolerant = map_iou.compare(shifted, truth, (0.0, 0.0, 0.0), tolerance_px=1)
    assert tolerant.iou > exact.iou


def test_load_map_lee_pgm_y_yaml_como_map_server(tmp_path):
    # 3 x 2 px: ocupado, desconocido (205), libre (254) / libre, ocupado, libre
    pixels = bytes([0, 205, 254, 255, 0, 254])
    (tmp_path / 'm.pgm').write_bytes(
        b'P5\n# CREATOR: test\n3 2\n255\n' + pixels)
    (tmp_path / 'm.yaml').write_text(
        'image: m.pgm\nmode: trinary\nresolution: 0.05\n'
        'origin: [-0.5, 1.0, 0]\nnegate: 0\n'
        'occupied_thresh: 0.65\nfree_thresh: 0.196\n')
    grid = map_iou.load_map(str(tmp_path / 'm.yaml'))
    assert grid.resolution == pytest.approx(0.05)
    assert grid.origin == pytest.approx((-0.5, 1.0, 0.0))
    np.testing.assert_array_equal(
        grid.state, [[OCC, UNK, FREE], [FREE, OCC, FREE]])
