# Chuleta del proyecto fuga

Todos los comandos se lanzan desde la raiz del workspace (`~/source/repos/fuga`).
En cada terminal nueva hay que cargar ROS (y el workspace si ya esta compilado):

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash        # solo tras compilar
```

## 1. Dependencias (rosdep)

rosdep lee los `package.xml` e instala con apt lo que falte. Ahora mismo **no esta
inicializado** en esta maquina; se hace una sola vez:

```bash
sudo rosdep init
rosdep update                    # repetir de vez en cuando para refrescar la base de datos
```

Comprobar e instalar. Hay que pasar `src` y `tools`, porque `maze_gen` (la libreria)
esta en `tools/maze_gen`; `--ignore-src` evita que intente instalar desde apt los
paquetes que ya estan en el workspace:

```bash
rosdep check   --from-paths src tools --ignore-src           # solo informa
rosdep install --from-paths src tools --ignore-src -r -y     # instala lo que falte
```

Regla: si un launch o el xacro empieza a usar un paquete nuevo (p. ej.
`teleop_twist_keyboard`, `slam_toolbox`), anadirlo como `<exec_depend>` en el
`package.xml` de ese paquete. Si no, rosdep no lo ve y en otra maquina falla al lanzar.

## 2. Compilar

```bash
# Primera vez o tras tocar maze_gen: --packages-up-to compila tambien sus dependencias
colcon build --packages-up-to maze_export fuga_description --symlink-install \
  --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
source install/setup.bash

# Solo el exportador de laberintos
colcon build --packages-select maze_export
ros2 run maze_export maze2world

# Todo lo necesario para el bringup (mundo + robot + SLAM + agentes + juez)
colcon build --packages-up-to fuga_bringup --symlink-install

# Solo agentes y juez
colcon build --packages-select fuga_agents fuga_eval
```

Paquetes del workspace:

| Paquete | Ruta | Que es |
|---|---|---|
| `maze_gen` | `tools/maze_gen` | Libreria C++ pura que genera laberintos (sin ROS) |
| `maze_export` | `src/maze_gen` | Nodo `maze2world`: laberinto -> `maze.sdf`, `maze.pgm`, `maze.yaml`, `meta.yaml` |
| `fuga_description` | `src/fuga_description` | Robot (xacro), puente Gazebo <-> ROS, RViz, `spawn.launch.py` |
| `fuga_agents` | `src/fuga_agents` | Agentes que escapan: seguidor de pared y Pledge |
| `fuga_slam` | `src/fuga_slam` | Parametros y launch de slam_toolbox |
| `fuga_bringup` | `src/fuga_bringup` | Launch que arranca mundo + robot + SLAM + agente + juez de una vez |
| `fuga_eval` | `src/fuga_eval` | Juez (`judge_node`): decide si el robot ha salido, en cuanto tiempo y con que recorrido |

Si `maze_export` falla con `maze_gen/maze_generator.hpp: No such file`, falta
`find_package(maze_gen REQUIRED)` en su `CMakeLists.txt` o no se ha compilado antes
`maze_gen`: usar `--packages-up-to maze_export`, que compila las dependencias en orden.

- `--symlink-install`: el xacro y los launch instalados son enlaces a `src/`, asi que
  cambiarlos no requiere recompilar (el C++ si). Si se cambia de modo (con/sin
  symlink), borrar antes `build/` e `install/`.
- Si algo se comporta raro tras renombrar ficheros o paquetes: compilacion limpia.

```bash
rm -rf build install log
```

## 3. Tests

```bash
colcon test --packages-select maze_gen maze_export fuga_description
colcon test-result --verbose     # colcon test no falla aunque fallen tests; mirar esto
```

`fuga_description` pasa los linters de `ament_lint_common` (xmllint, etc.).

### Formato del C++ (estilo ROS 2)

`colcon test` comprueba el formato con **uncrustify** (`ament_uncrustify`, de
`ament_lint_common`). El editor (clangd) formatea con **clang-format**, que lee
`.clang-format` de la raiz: es el oficial de ROS (`ament_clang_format`) mas unos
ajustes para parecerse a uncrustify. Casi todo coincide, pero no todo:

- Lambdas: uncrustify sangra su cuerpo 2 espacios mas que clang-format.
- `pi_v<float>/ 180.0f`: uncrustify quita el espacio antes de `/` tras un `>`.

clang-format deshace esas dos cosas si formatea el fichero entero. Por eso uncrustify
manda, y conviene:

```bash
ament_uncrustify src tools                  # comprobar (es lo que mira colcon test)
ament_uncrustify --reformat src tools       # arreglar, antes de hacer commit
```

y en VS Code formatear solo lo que se cambia (`"editor.formatOnSaveMode": "modifications"`)
en vez del fichero entero.

## 4. Comprobar el xacro

Cada paso detecta errores distintos; hacerlos en orden tras cada cambio:

```bash
xacro src/fuga_description/fuga.urdf.xacro -o /tmp/fuga.urdf   # 1. sintaxis xacro: propiedades, macros, ${...}
check_urdf /tmp/fuga.urdf                                      # 2. arbol links/joints: un solo root, sin huerfanos
gz sdf -p /tmp/fuga.urdf > /tmp/fuga.sdf                       # 3. conversion a SDF: lo que carga Gazebo de verdad
gz sdf -k /tmp/fuga.sdf                                        # 4. valida el SDF: debe acabar en "Valid."
grep -c "<sensor" /tmp/fuga.sdf                                # 5. deben salir 2 (imu_sensor y lidar_sensor)
urdf_to_graphviz /tmp/fuga.urdf /tmp/fuga                      # opcional: PDF con el arbol de frames
```

- **El paso 5 es el importante.** Un `<sensor>` o `<plugin>` que no este dentro de un
  `<gazebo>` pasa xacro y check_urdf, pero `gz sdf` lo descarta **sin avisar**.
  Sensores: `<gazebo reference="link">`. Plugins del modelo: `<gazebo>` sin reference.
- Los links unidos por joint `fixed` (lidar, imu_link, caster) se fusionan con
  `base_link` en el SDF. Es normal; por eso los sensores llevan `<gz_frame_id>`.
- Aviso esperado y sin importancia:
  `XML Element[gz_frame_id], child of element[sensor], not defined in SDF`.
  SDFormat no conoce la etiqueta, pero gz-sensors (Harmonic) la lee igualmente.
- Todo link con fisica (collision) necesita `<inertial>` con masa e inercia
  realistas; si no, Gazebo lo ignora o la simulacion se vuelve inestable.

## 5. Simulacion: laberinto + robot

### Todo de una vez (recomendado)

```bash
ros2 launch fuga_bringup bringup.launch.py                       # laberinto 10x10, robot, SLAM, wall_follower y juez
ros2 launch fuga_bringup bringup.launch.py agent:=pledge          # otro agente
ros2 launch fuga_bringup bringup.launch.py difficulty:=5 seed:=42  # mismo laberinto siempre
ros2 launch fuga_bringup bringup.launch.py slam:=false            # sin SLAM (agentes N1)
ros2 launch fuga_bringup bringup.launch.py agent:=none            # sin agente ni juez (teleop)
ros2 launch fuga_bringup bringup.launch.py --show-args            # todos los argumentos
```

Argumentos: los de `maze.launch.py` (`folder`, `width`, `height`, `difficulty`,
`seed`, `start_x`, `start_y`, `rtf`, `step`, `headless`), `rviz` de `spawn.launch.py`, `slam` (por defecto `true`),
`agent` (`wall_follower` por defecto, `pledge` o `none`) y `results` (CSV del juez,
por defecto `<folder>/results.csv`).
`start_x`/`start_y` es la celda de inicio (por defecto `0 0`): el bringup se la pasa
a `maze2world`, que mide desde ella las distancias de `meta.yaml`, y coloca el robot
en su centro. Por eso `x`/`y` ya no se pasan a mano en el bringup.

Orden de arranque dentro del bringup:
1. `maze.launch.py`: genera el laberinto y, cuando `maze2world` termina, abre Gazebo.
2. `spawn.launch.py`: arranca a la vez; el nodo `create` repite la peticion a
   Gazebo ("Requesting list of world names") hasta que responde.
3. `slam.launch.py`: cuando `create` termina, es decir, cuando el robot ya existe.
   Asi no hay avisos de TF por arrancar antes que el robot.
4. Agente y juez: tambien cuando `create` termina, los dos juntos (si `agent` no es
   `none`). Juntos para que el `start_timeout` del juez no corra mientras el agente
   aun no existe. El valor de `agent` va a los dos: al agente para elegir cual crear
   y al juez como `algorithm` para el CSV.
5. Cuando el juez termina (`exit`, `timeout` o `no_start`) se cierra todo
   (`OnProcessExit(juez) -> Shutdown`) y `ros2 launch` vuelve solo.

Cada pieza sigue siendo un launch independiente; el bringup solo las incluye. Asi
se puede lanzar o matar una sola (p. ej. matar SLAM en plena navegacion).

**Cuidado con una simulacion vieja abierta:** `create` mete el robot en el Gazebo que
encuentre. Si ya habia uno corriendo, aparece un segundo robot que publica en los
mismos `/odom` y `/tf` y obedece el mismo `/cmd_vel`. Antes de relanzar, cerrar
Gazebo (`pgrep -fa "gz sim"` para comprobarlo).

Paso de verdad: un mapa SLAM girado 18 grados y desplazado ~2 m respecto a la
verdad. El robot estaba en el Gazebo del bringup anterior, con otro laberinto
(`create` termino antes de que arrancara el Gazebo nuevo, y SLAM recibia scans con
hora de simulacion 14 s a los 4 s de lanzar).

Causa: `gz_sim.launch.py` lanza `sh -> ruby gz sim -> {gz sim server, gz sim gui}`.
Al cerrar, el launch solo mata el primero (`failed to terminate ... escalating to
SIGTERM`) y server y gui siguen vivos. El bringup ahora lo cubre de dos formas:
- Al arrancar, si ya hay un Gazebo, no lanza nada y sale con codigo 1
  (`Ya hay un Gazebo corriendo`). Un bucle de pruebas lo ve y puede parar.
- Cuando sale el proceso de Gazebo, mata los restos (`Gazebo ha dejado procesos
  vivos (server/gui): cerrandolos`); SIGINT y, si a los 3 s siguen, SIGKILL.

Si se cierra el launch a lo bruto (dos Ctrl+C, cerrar la terminal) la limpieza no
llega a ejecutarse; el siguiente bringup avisara. A mano:
`pkill -f '(^|/)gz sim( |$)'` (el patron evita matar shells con "gz sim" en el texto).

### Por partes (para depurar)

```bash
# Terminal 1: genera el laberinto en /tmp/maze (maze.sdf, maze.pgm, maze.yaml...) y abre Gazebo con el
ros2 launch maze_export maze.launch.py            # otra carpeta: folder:=/ruta; otro inicio: start_x:=1 start_y:=0

# Terminal 2: con Gazebo ya abierto, mete el robot en la celda de inicio (0.5, 0.5)
ros2 launch fuga_description spawn.launch.py      # otra posicion: x:=1.5 y:=0.5 (debe coincidir con start de meta.yaml)

# Terminal 3: mover el robot (Ctrl+C no lo para; mandar velocidad 0)
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist '{linear: {x: 0.2}}'
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist '{}'
# Mas comodo: teclado (publica en /cmd_vel)
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# Terminal 4: agente. El primer argumento elige cual: wall_follower (por defecto) o pledge
ros2 run fuga_agents fuga_agents pledge --ros-args -p use_sim_time:=true
ros2 run fuga_agents fuga_agents wall_follower --ros-args -p use_sim_time:=true -p side:=left -p velocity:=0.2

# Terminal 5: juez, a la vez que el agente. Al terminar escribe el CSV y sale
ros2 run fuga_eval judge_node --ros-args -p algorithm:=pledge
```

El agente es un argumento y no un parametro ROS porque hay que saberlo antes de crear
el nodo. `main.cpp` lo saca con `rclcpp::remove_ros_arguments()`, que quita los
`--ros-args ...`. Con un nombre desconocido sale con error en vez de lanzar otro:
el juez apuntaria en el CSV un agente que no es.

Desde la propia ventana de Gazebo, sin ROS: menu de plugins (los tres puntos, arriba
a la derecha) > **Key Publisher**, hacer clic en la escena y usar las flechas (espacio
para). Lo hacen los `TriggeredPublisher` del xacro. Ver que llegan las teclas:

```bash
gz topic -e -t /keyboard/keypress             # flechas: 16777234-16777237, espacio: 32
```

Tras cambiar el xacro hay que volver a spawnear el robot: lo mas sencillo es cerrar
los dos launch y relanzarlos.

## 6. Comprobar sensores y puente (con la simulacion corriendo)

El lidar publica en `/lidar` en Gazebo y el puente lo renombra a `/scan` en ROS
(el nombre que esperan slam_toolbox y nav2).

Primero en Gazebo y luego en ROS. Si un topic sale en gz pero no en ROS, falla el
puente (`spawn.launch.py`); si no sale ni en gz, falla el sensor o falta el sistema
en el mundo (`Sensors` / `Imu`, los anade `maze_export`).

```bash
gz topic -l                                   # lado Gazebo: deben salir /imu y /lidar
gz topic -e -t /imu -n 1                      # un mensaje directo de Gazebo

ros2 topic list                               # lado ROS
ros2 topic echo /imu --once                   # frame_id: imu_link; parado, linear_acceleration.z ~ 9.8
ros2 topic echo /scan --once --field header   # frame_id: lidar
ros2 topic hz /imu                            # ~100 Hz
ros2 topic hz /scan                           # ~10 Hz
ros2 topic echo /ground_truth/pose --once     # pose real en el mundo, frame_id: maze
rqt_graph                                     # nodos y topics conectados
```

**WSL2 y gpu_lidar:** el lidar se calcula con el motor de render (ogre2). Si `/lidar`
no publica o Gazebo se cierra al cargar el robot, probar con render por software
(lento, pero funciona):

```bash
export LIBGL_ALWAYS_SOFTWARE=1                # en la terminal de maze.launch.py, antes de lanzar
```

## 7. TF y RViz

```bash
ros2 run tf2_tools view_frames                # frames.pdf: map -> odom -> base_link -> ruedas, caster, lidar, imu_link
ros2 run tf2_ros tf2_echo base_link lidar     # debe dar traslacion 0 0 0.05
rviz2 --ros-args -p use_sim_time:=true
```

Quien publica cada flecha de TF:

| Transformada | La publica |
|---|---|
| `map -> odom` | slam_toolbox (solo con SLAM) |
| `odom -> base_link` | plugin DiffDrive de Gazebo, por el puente `/tf` |
| `base_link -> lidar`, `imu_link`, `caster` | robot_state_publisher (joints fixed del URDF) |
| `base_link -> ruedas` | robot_state_publisher, a partir de `/joint_states` |

`spawn.launch.py` abre RViz con `fuga_description/config/fuga.rviz`: Fixed Frame
`map`, `Map` (`/map`), `RobotModel`, `TF` y `LaserScan` (`/scan`). **Con `slam:=false`
no existe `map`**: cambiar el Fixed Frame a `odom` a mano en RViz.

**use_sim_time:** todo nodo que se anada (RViz, nav2, SLAM...) debe llevar
`use_sim_time:=true`. Los mensajes llevan la hora de Gazebo (`/clock`); con la hora
del sistema los descarta por antiguos y TF da errores de extrapolacion.

## 8. SLAM (slam_toolbox)

slam_toolbox en modo online asincrono construye el mapa mientras el robot se mueve.
Lo lanza el bringup; por separado (con mundo y robot ya corriendo):

```bash
ros2 launch fuga_slam slam.launch.py
ros2 launch fuga_slam slam.launch.py slam_params_file:=/ruta/otro.yaml   # probar otros parametros
```

Parametros en `src/fuga_slam/config/mapper_params_online_async.yaml`. Cambiados
respecto al de slam_toolbox:

- `base_frame: base_link`. El de serie es `base_footprint`, que este robot no tiene;
  sin el cambio salen `Message Filter dropping message` y no hay mapa.
- `max_laser_range: 10.0`, el alcance del lidar del xacro.
- `resolution: 0.05` se deja igual a proposito: coincide con `maze.pgm`
  (20 px por celda de 1 m), asi el IoU no necesita remuestrear.

Guardar el mapa (genera `.pgm` + `.yaml`; necesita `nav2_map_server`):

```bash
ros2 run nav2_map_server map_saver_cli -f ~/maps/n1_slam --ros-args -p use_sim_time:=true
```

El origen del mapa SLAM es la pose de spawn (0.5, 0.5), no la esquina del laberinto
como en `maze.yaml`. Hay que tenerlo en cuenta al comparar con el PGM verdad.

Comparar con el PGM verdad (`scripts/map_iou.py`). Alinea el mapa con el `start` de
`meta.yaml` (yaw 0, como el spawn) y da el IoU de las celdas ocupadas:

```bash
ros2 run fuga_slam map_iou.py --slam ~/maps/n1_slam.yaml            # verdad: /tmp/maze/maze.yaml
ros2 run fuga_slam map_iou.py --slam ~/maps/n1_slam.yaml --tolerance 0.05 --diff /tmp/n1_diff.png
ros2 run fuga_slam map_iou.py --slam ~/maps/n1_slam.yaml --json      # para scripts
```

- `IoU`: sobre todo el laberinto. Si el agente no lo recorre entero sale bajo aunque
  el mapa sea bueno. `IoU explorado`: solo donde SLAM ha visto algo (+ 0.1 m).
- `precision`: muros de SLAM que existen. `recall`: muros verdad que SLAM ha visto.
- `--tolerance` (m) amplia los muros de los dos antes de comparar. Celda a celda
  (0) penaliza mucho: SLAM pinta la cara del muro (1 px) y la verdad el muro entero
  (2 px), y hay desfases de 1 px por deriva o redondeo.
- `--diff`: PNG. Negro, coinciden; rojo, solo SLAM; azul, solo verdad; gris, sin
  explorar.
- **El mapa y `/tmp/maze` deben ser del mismo laberinto**: cada bringup lo regenera
  (otra `seed` si es 0). Comprobar la `seed` de `meta.yaml`.

Comprobar que funciona:

```bash
ros2 run tf2_ros tf2_echo map odom    # al principio identidad; despues cambia poco y despacio
ros2 topic hz /map                    # una vez cada map_update_interval (5 s)
```

**Diagnostico si `map -> odom` es grande** (metros o decenas de grados): comparar
las tres poses **con el robot parado** (si se mueve, las lecturas no son del mismo
instante):

```bash
ros2 topic echo /ground_truth/pose --once     # verdad, en coordenadas de mundo
ros2 run tf2_ros tf2_echo odom base_link      # lo que cree la odometria
ros2 run tf2_ros tf2_echo map base_link       # lo que cree SLAM
```

`odom` y `map` nacen en el spawn: restar (0.5, 0.5) a la verdad antes de comparar.
En un laberinto de 10x10 ninguna de las dos puede pasar de ~9.5 m.

- `map` ~ verdad y `odom` lejos: la odometria deriva y SLAM la corrige (bien).
- `odom` ~ verdad y `map` lejos: SLAM perdido (muros duplicados en RViz). Suele ser
  aliasing: todos los pasillos de 1 m se parecen.
- Los dos lejos: la odometria deriva tanto que el scan matching ya no la recupera.

DiffDrive calcula la odometria con la velocidad de las ruedas, no con el movimiento
real: si el robot empuja un muro o patina, la odometria sigue sumando. Para aislarla:
`slam:=false`, teleop, y comparar con la verdad tras una recta de 2 m, un giro de
360 grados y unos segundos empujando un muro.

## 9. Juez (fuga_eval)

Decide si una ejecucion ha terminado y como: si el robot ha salido del laberinto,
en cuanto tiempo y con que longitud de camino. Escribe una fila por ejecucion en
un CSV, para comparar agentes y laberintos.

**Estado:** libreria (`fuga_eval_lib`) y nodo (`judge_node`) compilan, y el
bringup los lanza. Probado el nodo con poses publicadas a mano (`no_start`); falta
una ejecucion completa con Gazebo.

### Libreria (sin ROS)

| Fichero | Que hace |
|---|---|
| `judge.hpp` | `Judge`: maquina de estados ESPERANDO -> CORRIENDO -> TERMINADO |
| `meta.hpp` | `load_meta()`: lee `meta.yaml` (bounds, tamano, seed, camino mas corto) |
| `geometry.hpp` | `Point`, `Bounds`, `is_outside()` |
| `path_length.hpp` | Suma la distancia recorrida; solo cuenta un tramo cuando la pose se ha alejado `min_step` del ultimo punto contado, para que el ruido no la infle |
| `csv.hpp` | Cabecera, filas y `append_row()` del CSV de resultados |

`Judge::update(t, p)` recibe cada pose con su hora:
- ESPERANDO: guarda la primera pose. Pasa a CORRIENDO cuando el robot se aleja mas
  de `start_threshold` (0.02 m); si no lo hace en `start_timeout` (30 s) -> `no_start`.
- CORRIENDO: acumula distancia. `exit` la primera vez que la pose queda fuera de
  `bounds` + `exit_margin` (0.2 m); `timeout` si pasan `timeout` (600 s).

Columnas del CSV: `date,algorithm,seed,width,height,difficulty,result,time_s,`
`path_length_m,shortest_m,exit_x,exit_y`.

### Nodo (`judge_node`)

Adaptador fino entre ROS y la libreria:
- `JudgeConfig` sale de dos sitios: `bounds` de `meta.yaml` (con `load_meta()`), y
  `timeout`, `start_timeout`, `exit_margin`, `start_threshold` y `min_step` de
  parametros ROS. `meta.yaml` describe el laberinto; los tiempos y margenes son del juez.
- Otros parametros: `meta_file` (`/tmp/maze/meta.yaml`), `csv_file`
  (`/tmp/maze/results.csv`) y `algorithm` (nombre del agente para el CSV; el juez
  no puede adivinarlo).
- Se suscribe a `/ground_truth/pose`. **No a `/odom`**: la odometria deriva (ver
  seccion 8) y `meta.bounds` esta en coordenadas de mundo, como la verdad.
- La hora sale de `header.stamp` (hora de simulacion), no de `now()`, y con
  `rclcpp::Time(stamp).seconds()`. Solo `stamp.sec` redondea a segundos enteros y
  solo `stamp.nanosec` es la parte fraccionaria (vuelve a 0 cada segundo).
- Al terminar: log, fila en el CSV, mapa SLAM (ver abajo) y `rclcpp::shutdown()`,
  que hace que `spin()` vuelva y el proceso salga. Si `meta.yaml` no existe o esta mal: log `FATAL` y
  sale con codigo 1 (el bringup tambien se cierra).
- Arrancarlo a la vez que el agente: la primera pose que recibe es el origen, y
  `start_timeout` (30 s) cuenta desde ella.

**Mapa al terminar** (`save_map`, por defecto `true`; el bringup le pasa `slam`):
tras escribir el CSV, el juez lanza `map_saver_cli` y luego `map_iou.py --json`
(seccion 8) y deja en `map_dir` (`/tmp/maze/maps`; en el bringup, argumento `maps`):

```
<fecha>_<agente>_seed<seed>.pgm / .yaml   mapa SLAM
<fecha>_<agente>_seed<seed>_diff.png      comparacion con maze.pgm
<fecha>_<agente>_seed<seed>_iou.json      IoU, IoU explorado, precision, recall
```

- Se hace **antes** del `shutdown()` a proposito: el bringup cierra SLAM cuando sale
  el juez, asi que `/map` solo existe mientras el juez siga vivo. Tarda ~2 s.
- La verdad es el `maze.yaml` junto a `meta_file`: se compara en el momento, antes
  de que el siguiente bringup regenere `/tmp/maze`.
- `iou_tolerance` (m, por defecto 0) es el `--tolerance` de `map_iou.py`.
- Sin `/map` (SLAM caido o `slam:=false` con `save_map:=true`), `map_saver_cli`
  falla a los ~2 s: log de error y el juez termina igual.
- El mapa es el ultimo `/map` publicado, que llega cada `map_update_interval`
  (5 s): pueden faltar los ultimos segundos antes de salir.
- Los comandos van por `ros2 run` (`map_capture.hpp`, rutas entre comillas
  simples): el juez necesita el entorno de ROS cargado, como cualquier nodo.

Callback de un metodo: `std::bind(&JudgeNode::callback, this, std::placeholders::_1)`.
El metodo necesita el objeto (`this`); `_1` es el hueco donde ROS pone el mensaje.
El `10` de `create_subscription` es la QoS: con un numero, perfil por defecto
(`reliable`, `volatile`) y cola de 10 mensajes. Publicador y suscriptor tienen que
ser compatibles (`best_effort` contra `reliable` no conecta y no da error).

### Baterias de pruebas

Cada `ros2 launch` vuelve solo cuando el juez termina, asi que basta un bucle. Cada
ejecucion anade una fila al mismo CSV:

```bash
for seed in 1 2 3; do
  for agent in wall_follower pledge; do
    ros2 launch fuga_bringup bringup.launch.py seed:=$seed agent:=$agent \
      rviz:=false headless:=true rtf:=10 step:=0.005 slam:=false \
      results:=$HOME/fuga_results.csv
  done
done
```

`results` fuera de `/tmp` para no perderlo al reiniciar.

### Acelerar la simulacion

```bash
ros2 launch fuga_bringup bringup.launch.py rtf:=10 step:=0.005 headless:=true rviz:=false
```

- `step`: paso de fisica (s). `maze2world` lo escribe en el `<physics>` de
  `maze.sdf` (`max_step_size`). Por defecto 0.001, el de Gazebo.
- `rtf`: maximo de veces mas rapido que el reloj real. El launch pasa
  `gz sim -z <rtf / step>` (pasos de fisica por segundo real).
- `headless:=true`: solo `gz sim server` (`-s`), sin GUI; tampoco pide a la GUI que
  la camara siga al robot.
- Es seguro porque todo va con hora de simulacion: el agente reacciona a cada
  `/scan`, el juez mide con el `stamp` de `/ground_truth/pose` y `exit_timeout`,
  `timeout` y `start_timeout` son segundos simulados.
- Factor real: `gz topic -e -t /stats` (campo `real_time_factor`).

**Por que `rtf` solo no basta.** Cada paso de fisica tiene un coste fijo
(~0.3 ms el laberinto solo, ~0.65 ms con el robot, medido en WSL2 con
`gz sim -s` y `-z` alto). Con pasos de 1 ms el techo es ~1.5x aunque se pida 10x, y
la CPU queda medio vacia: no es falta de potencia, es trabajo en serie por paso. El
lidar no es el freno (sin el sale igual), aunque renderiza por software (llvmpipe).
Medido con `gz sim -s` solo (sin ROS):

| paso  | rtf maximo |
|-------|------------|
| 1 ms  | ~1.5x      |
| 2 ms  | ~2.9x      |
| 3 ms  | ~4.7x      |
| 5 ms  | ~6.2x      |

Con todo el bringup (puente, agente, juez) se queda en ~5x con 5 ms.

**Por que 5 ms es seguro.** Mismo arco (0.2 m/s, 0.4 rad/s, 20 s) con pasos de 1 a
5 ms: el robot no rebota (z estable) y la pose final queda igual o mas cerca de la
teorica. Bringup con `seed:=42`, 5x5: 93.2 s / 26.47 m a 1 ms y 93.5 s / 26.49 m a
5 ms; con SLAM el IoU no empeora (0.35 a 1 ms, 0.45 a 5 ms, una ejecucion de cada).
Mejor un paso que divida los periodos de los sensores (IMU 10 ms, pose 20 ms, lidar
50 ms): 1, 2 o 5 ms. Con 3 o 4 ms salen a intervalos irregulares.

**JointStatePublisher** lleva `<update_rate>30</update_rate>`: sin el publica
`/joint_states` en cada paso de fisica, y el puente y robot_state_publisher gastaban
CPU que frenaba la simulacion acelerada (46 s -> 40 s en la prueba de arriba).
`/clock` sigue saliendo en cada paso; Gazebo no tiene opcion para espaciarlo.

## 10. Pendiente

- `package.xml`: rellenar `<description>` en `fuga_description`, `fuga_agents`,
  `fuga_slam` y `fuga_eval`, y `<license>` en `maze_export` (siguen con TODO).
- Probar el bringup con agente + juez de principio a fin en Gazebo (seccion 9).
- Tests gtest de `fuga_eval_lib`: solo hay `test/test_map_capture.cpp`; faltan
  `Judge`, `PathLength`, `csv` y `meta`.
- Tamano de celda (1 m) duplicado: `kCell` en `maze_export/export.hpp` y el `+ 0.5`
  con que `bringup.launch.py` pasa de `start_x`/`start_y` a la posicion del spawn.
  Si cambia, tocar los dos (o que el launch lo lea de un sitio comun).
- Avisos de flake8 previos: E122 (sangria) en `spawn.launch.py:75` (el `]))` de
  `follow_camera`), que hace fallar el test de `fuga_description`, y D403 en el
  docstring de `include()` de `bringup.launch.py` (primera palabra en minuscula).
- Diagnosticar la deriva de la odometria (seccion 8).
- RViz: un `.rviz` para `slam:=false` con Fixed Frame `odom`.
- Nav2 y cliente de `NavigateToPose`.
- No instalados aun: `robot_localization` (fusionar IMU + odometria) y `nav2`
  completo (`ros-jazzy-navigation2 ros-jazzy-nav2-bringup`). Al usarlos:
  `exec_depend` en el package.xml y `rosdep install`.
