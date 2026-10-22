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
```

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

```bash
# Terminal 1: genera el laberinto en /tmp/maze (maze.sdf, maze.pgm, maze.yaml...) y abre Gazebo con el
ros2 launch maze_export maze.launch.py            # otra carpeta: folder:=/ruta

# Terminal 2: con Gazebo ya abierto, mete el robot en la celda de salida (0.5, 0.5)
ros2 launch fuga_description spawn.launch.py      # otra posicion: x:=1.5 y:=0.5

# Terminal 3: mover el robot (Ctrl+C no lo para; mandar velocidad 0)
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist '{linear: {x: 0.2}}'
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist '{}'
# Mas comodo: teclado (publica en /cmd_vel)
ros2 run teleop_twist_keyboard teleop_twist_keyboard

# Terminal 4: fuga del laberinto con la regla de la mano derecha
ros2 run fuga_agents fuga_agents
```

Desde la propia ventana de Gazebo, sin ROS: menu de plugins (los tres puntos, arriba
a la derecha) > **Key Publisher**, hacer clic en la escena y usar las flechas (espacio
para). Lo hacen los `TriggeredPublisher` del xacro. Ver que llegan las teclas:

```bash
gz topic -e -t /keyboard/keypress             # flechas: 16777234-16777237, espacio: 32
```

Tras cambiar el xacro hay que volver a spawnear el robot: lo mas sencillo es cerrar
los dos launch y relanzarlos.

## 6. Comprobar sensores y puente (con la simulacion corriendo)

Primero en Gazebo y luego en ROS. Si un topic sale en gz pero no en ROS, falla el
puente (`spawn.launch.py`); si no sale ni en gz, falla el sensor o falta el sistema
en el mundo (`Sensors` / `Imu`, los anade `maze_export`).

```bash
gz topic -l                                   # lado Gazebo: deben salir /imu y /lidar
gz topic -e -t /imu -n 1                      # un mensaje directo de Gazebo

ros2 topic list                               # lado ROS
ros2 topic echo /imu --once                   # frame_id: imu_link; parado, linear_acceleration.z ~ 9.8
ros2 topic echo /lidar --once --field header  # frame_id: lidar
ros2 topic hz /imu                            # ~100 Hz
ros2 topic hz /lidar                          # ~10 Hz
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
ros2 run tf2_tools view_frames                # frames.pdf: odom -> base_link -> ruedas, caster, lidar, imu_link
ros2 run tf2_ros tf2_echo base_link lidar     # debe dar traslacion 0 0 0.05
rviz2 --ros-args -p use_sim_time:=true
```

En RViz: Fixed Frame `odom`; anadir `RobotModel` (Description Topic `/robot_description`),
`TF` y `LaserScan` (topic `/lidar`).

**use_sim_time:** todo nodo que se anada (RViz, nav2, SLAM...) debe llevar
`use_sim_time:=true`. Los mensajes llevan la hora de Gazebo (`/clock`); con la hora
del sistema los descarta por antiguos y TF da errores de extrapolacion.

## 8. Pendiente

- `package.xml`: rellenar `<description>` en `fuga_description` y `<license>` en
  `maze_export` (siguen con TODO).
- Topic del lidar: slam_toolbox y nav2 esperan `/scan` por defecto. O se renombra el
  topic a `scan` en el xacro y el puente, o se remapea al lanzarlos.
- No instalados aun: `slam_toolbox` (mapa), `robot_localization` (fusionar IMU +
  odometria), `nav2`. Al usarlos: `exec_depend` en el package.xml y `rosdep install`.
