#!/usr/bin/env python3
"""
Compara un mapa de SLAM con el PGM verdad del laberinto: IoU de muros.

El mapa SLAM (guardado con map_saver_cli) esta en el frame map, que nace en la
pose de spawn. El verdad (maze.yaml de maze2world) esta en coordenadas de
mundo, con origen en la esquina del laberinto. Para alinearlos se usa la pose
de inicio: p_mundo = R(yaw) * p_map + start, con start sacado de meta.yaml.

Las dos rejillas se comparan sobre una rejilla comun con la resolucion de la
verdad, que cubre los dos mapas: un muro de SLAM fuera del laberinto cuenta
como falso positivo. Cada celda toma el valor de la celda mas cercana de cada
mapa (vecino mas proximo), asi que vale tambien con yaw o resolucion distintos.

Metricas sobre las celdas ocupadas:
  IoU             |SLAM y verdad| / |SLAM o verdad|, en toda la rejilla
  IoU explorado   lo mismo, solo donde SLAM sabe algo (libre u ocupado), con un
                  margen de un grosor de muro: el robot que no recorre todo el
                  laberinto no es penalizado por los muros que no ha visto
  precision       que parte de los muros de SLAM existe de verdad
  recall          que parte de los muros verdad ha encontrado SLAM

Uso:
  ros2 run fuga_slam map_iou.py --slam ~/maps/n1_slam.yaml
  ros2 run fuga_slam map_iou.py --slam ~/maps/n1_slam.yaml --tolerance 0.05
  ros2 run fuga_slam map_iou.py --diff /tmp/n1_diff.png --json
"""
import argparse
from dataclasses import asdict, dataclass
import json
import math
import os
import sys

import numpy as np
import yaml

# Estado de cada celda, como el OccupancyGrid de ROS: -1 desconocido
UNKNOWN, FREE, OCCUPIED = -1, 0, 1


@dataclass
class GridMap:
    """
    Rejilla de ocupacion en el formato de nav2_map_server.

    state: filas de arriba abajo, como en el PGM (fila 0 = y maxima).
    origin: (x, y, yaw) de la esquina inferior izquierda, en su frame.
    """

    state: np.ndarray
    resolution: float
    origin: tuple


@dataclass
class Result:
    iou: float
    explored_iou: float
    precision: float
    recall: float
    true_positives: int
    false_positives: int
    false_negatives: int
    slam_occupied: int
    truth_occupied: int
    explored_cells: int
    tolerance_px: int


def read_pgm(path):
    """PGM binario (P5) de 8 bits, admitiendo comentarios en la cabecera."""
    with open(path, 'rb') as f:
        data = f.read()
    fields, pos = [], 0
    while len(fields) < 4:
        while data[pos:pos + 1].isspace():
            pos += 1
        if data[pos:pos + 1] == b'#':
            pos = data.index(b'\n', pos) + 1
            continue
        end = pos
        while not data[end:end + 1].isspace():
            end += 1
        fields.append(data[pos:end])
        pos = end
    magic, width, height, maxval = fields[0], *map(int, fields[1:])
    if magic != b'P5' or maxval > 255:
        raise ValueError(f'{path}: solo se admite PGM P5 de 8 bits')
    pos += 1  # un unico espacio separa la cabecera de los pixeles
    pixels = np.frombuffer(data, dtype=np.uint8, count=width * height,
                           offset=pos)
    return pixels.reshape(height, width)


def load_map(yaml_path):
    """Lee un mapa (yaml + pgm) e interpreta sus pixeles como map_server."""
    with open(yaml_path) as f:
        meta = yaml.safe_load(f)
    image = os.path.join(os.path.dirname(os.path.abspath(yaml_path)),
                         meta['image'])
    pixels = read_pgm(image).astype(np.float64)
    # Probabilidad de ocupado: 0 (negro) es ocupado salvo con negate
    p = pixels / 255.0 if meta.get('negate', 0) else (255.0 - pixels) / 255.0
    state = np.full(pixels.shape, UNKNOWN, dtype=np.int8)
    state[p > float(meta['occupied_thresh'])] = OCCUPIED
    state[p < float(meta['free_thresh'])] = FREE
    origin = tuple(float(v) for v in meta['origin'])
    return GridMap(state, float(meta['resolution']), origin)


def load_start(meta_path, yaw=0.0):
    """
    Pose de inicio (x, y, yaw) en mundo, de meta.yaml.

    meta.yaml no guarda el yaw: el spawn no lo cambia, asi que es 0.
    """
    with open(meta_path) as f:
        meta = yaml.safe_load(f)
    x, y = meta['start']
    return (float(x), float(y), float(yaw))


def rotate(x, y, angle):
    c, s = math.cos(angle), math.sin(angle)
    return c * x - s * y, s * x + c * y


def sample(grid, x, y):
    """Estado de la celda de grid que contiene cada punto (x, y) de su frame."""
    ox, oy, oyaw = grid.origin
    lx, ly = rotate(x - ox, y - oy, -oyaw)
    rows, cols = grid.state.shape
    col = np.floor(lx / grid.resolution).astype(np.int64)
    row = rows - 1 - np.floor(ly / grid.resolution).astype(np.int64)
    inside = (col >= 0) & (col < cols) & (row >= 0) & (row < rows)
    out = np.full(x.shape, UNKNOWN, dtype=np.int8)
    out[inside] = grid.state[row[inside], col[inside]]
    return out


def corners(grid):
    """Esquinas del mapa en su frame (tiene en cuenta el yaw del origen)."""
    rows, cols = grid.state.shape
    w, h = cols * grid.resolution, rows * grid.resolution
    ox, oy, oyaw = grid.origin
    return [(ox + dx, oy + dy) for dx, dy in
            (rotate(px, py, oyaw) for px, py in
             ((0, 0), (w, 0), (0, h), (w, h)))]


def common_grid(slam, truth, start):
    """
    Centros (x, y) en mundo de una rejilla alineada con la verdad.

    Se amplia la verdad, a su misma resolucion, hasta cubrir tambien el mapa
    SLAM puesto en mundo.
    """
    res = truth.resolution
    tx0, ty0, _ = truth.origin
    rows, cols = truth.state.shape
    sx, sy, syaw = start
    world = [rotate(x, y, syaw) for x, y in corners(slam)]
    xs = [sx + x for x, _ in world]
    ys = [sy + y for _, y in world]
    left = max(0, math.ceil((tx0 - min(xs)) / res))
    right = max(0, math.ceil((max(xs) - (tx0 + cols * res)) / res))
    bottom = max(0, math.ceil((ty0 - min(ys)) / res))
    top = max(0, math.ceil((max(ys) - (ty0 + rows * res)) / res))
    n_cols, n_rows = cols + left + right, rows + bottom + top
    gx0, gy0 = tx0 - left * res, ty0 - bottom * res
    cx = gx0 + (np.arange(n_cols) + 0.5) * res
    cy = gy0 + (n_rows - 1 - np.arange(n_rows) + 0.5) * res  # filas arriba-abajo
    return np.meshgrid(cx, cy)


def dilate(mask, radius):
    """Amplia mask con un disco de radius celdas (sin scipy)."""
    if radius <= 0:
        return mask
    padded = np.pad(mask, radius)
    out = np.zeros_like(mask)
    rows, cols = mask.shape
    for dr in range(-radius, radius + 1):
        for dc in range(-radius, radius + 1):
            if dr * dr + dc * dc <= radius * radius:
                out |= padded[radius + dr:radius + dr + rows,
                              radius + dc:radius + dc + cols]
    return out


def iou(a, b):
    union = np.count_nonzero(a | b)
    return np.count_nonzero(a & b) / union if union else float('nan')


def ratio(num, den):
    return num / den if den else float('nan')


def compare(slam, truth, start, tolerance_px=0, explored_margin_px=None,
            layers=None):
    """
    Compara slam (frame map) con truth (mundo); start = (x, y, yaw) en mundo.

    tolerance_px amplia los muros de los dos mapas antes de compararlos, para
    no penalizar desfases de una o dos celdas (deriva, muros de 1 px frente a
    2 px). explored_margin_px es el margen alrededor de lo conocido por SLAM;
    por defecto, 0.1 m (el grosor de muro de maze2world).
    Si layers es un dict, se rellena con las mascaras, para dibujarlas.
    """
    if explored_margin_px is None:
        explored_margin_px = round(0.1 / truth.resolution)
    wx, wy = common_grid(slam, truth, start)
    sx, sy, syaw = start
    mx, my = rotate(wx - sx, wy - sy, -syaw)
    slam_state = sample(slam, mx, my)
    truth_state = sample(truth, wx, wy)

    s_occ = slam_state == OCCUPIED
    t_occ = truth_state == OCCUPIED
    explored = dilate(slam_state != UNKNOWN, explored_margin_px)
    s_tol = dilate(s_occ, tolerance_px)
    t_tol = dilate(t_occ, tolerance_px)

    tp = np.count_nonzero(s_occ & t_tol)   # muro SLAM con muro verdad cerca
    fp = np.count_nonzero(s_occ & ~t_tol)  # muro SLAM donde no hay nada
    fn = np.count_nonzero(t_occ & ~s_tol)  # muro verdad que SLAM no tiene
    if layers is not None:
        layers.update(slam=s_occ, truth=t_occ, explored=explored,
                      fp=s_occ & ~t_tol, fn=t_occ & ~s_tol)
    return Result(
        iou=iou(s_tol, t_tol),
        explored_iou=iou(s_tol & explored, t_tol & explored),
        precision=ratio(tp, np.count_nonzero(s_occ)),
        recall=ratio(np.count_nonzero(t_occ & s_tol), np.count_nonzero(t_occ)),
        true_positives=tp,
        false_positives=fp,
        false_negatives=fn,
        slam_occupied=int(np.count_nonzero(s_occ)),
        truth_occupied=int(np.count_nonzero(t_occ)),
        explored_cells=int(np.count_nonzero(slam_state != UNKNOWN)),
        tolerance_px=tolerance_px,
    )


def save_diff(path, layers, scale=4):
    """
    PNG con la comparacion (arriba = y maxima, como en RViz desde arriba).

    negro: muro en los dos; rojo: solo SLAM (falso positivo); azul: solo
    verdad (no encontrado); gris: fuera de lo explorado; blanco: libre.
    """
    from PIL import Image

    rows, cols = layers['slam'].shape
    rgb = np.full((rows, cols, 3), 255, dtype=np.uint8)
    rgb[~layers['explored']] = (215, 215, 215)
    rgb[layers['fn']] = (40, 90, 220)
    rgb[layers['fp']] = (220, 40, 40)
    rgb[(layers['slam'] | layers['truth']) & ~layers['fp'] & ~layers['fn']] = 0
    image = Image.fromarray(rgb).resize((cols * scale, rows * scale),
                                        Image.NEAREST)
    image.save(path)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description='IoU de muros entre el mapa SLAM y el PGM verdad.')
    parser.add_argument('--slam', default=os.path.expanduser(
        '~/maps/n1_slam.yaml'), help='yaml de map_saver_cli (frame map)')
    parser.add_argument('--truth', default='/tmp/maze/maze.yaml',
                        help='maze.yaml de maze2world (mundo)')
    parser.add_argument('--meta', default=None,
                        help='meta.yaml con start; por defecto, junto a --truth')
    parser.add_argument('--start', nargs=2, type=float, metavar=('X', 'Y'),
                        help='pose de inicio en mundo (m); sustituye a meta.yaml')
    parser.add_argument('--yaw', type=float, default=0.0,
                        help='yaw de inicio (rad); el spawn usa 0')
    parser.add_argument('--tolerance', type=float, default=0.0,
                        help='margen (m) con el que dos muros cuentan como el '
                             'mismo; 0 = celda a celda')
    parser.add_argument('--diff', help='PNG donde dibujar la comparacion')
    parser.add_argument('--json', action='store_true',
                        help='resultado en JSON, para scripts')
    args = parser.parse_args(argv)

    truth = load_map(args.truth)
    slam = load_map(args.slam)
    if args.start:
        start = (args.start[0], args.start[1], args.yaw)
    else:
        meta = args.meta or os.path.join(
            os.path.dirname(os.path.abspath(args.truth)), 'meta.yaml')
        start = load_start(meta, args.yaw)
    tolerance_px = round(args.tolerance / truth.resolution)

    layers = {} if args.diff else None
    result = compare(slam, truth, start, tolerance_px, layers=layers)
    if args.diff:
        save_diff(args.diff, layers)

    if args.json:
        print(json.dumps({**asdict(result), 'start': start}))
        return 0
    if not math.isclose(slam.resolution, truth.resolution):
        print(f'Aviso: resoluciones distintas (SLAM {slam.resolution}, verdad '
              f'{truth.resolution}); se remuestrea al vecino mas cercano',
              file=sys.stderr)
    print(f'Inicio (mundo): x={start[0]:.3f} y={start[1]:.3f} '
          f'yaw={start[2]:.3f}')
    print(f'Tolerancia:     {tolerance_px} celdas '
          f'({tolerance_px * truth.resolution:.2f} m)')
    print(f'Celdas ocupadas: SLAM {result.slam_occupied}, '
          f'verdad {result.truth_occupied}')
    print(f'IoU:            {result.iou:.3f}')
    print(f'IoU explorado:  {result.explored_iou:.3f}')
    print(f'Precision:      {result.precision:.3f}  '
          f'({result.false_positives} muros SLAM sin muro verdad)')
    print(f'Recall:         {result.recall:.3f}  '
          f'({result.false_negatives} muros verdad sin encontrar)')
    if args.diff:
        print(f'Diferencias:    {args.diff}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
