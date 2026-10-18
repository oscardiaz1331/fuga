# Funciones nuevas

Ideas para ampliar `maze_gen`: las que ya están incorporadas y las que quedan. Todas mantienen la librería sin dependencias de ROS; lo que necesite mensajes de ROS va en el paquete que la use.

| Id | Función | Para qué | Estado |
|----|---------|----------|--------|
| [F1](#f1-convertir-el-laberinto-en-una-rejilla-de-ocupación) | Rejilla de ocupación | Mapas de navegación | Pendiente |
| [F2](#f2-extraer-los-muros-como-segmentos) | Muros como segmentos | Mundo de simulación, visualización | Pendiente |
| [F3](#f3-resolver-y-medir-el-laberinto) | Resolver y medir | Validar, elegir inicio y meta | Pendiente |
| [F4](#f4-dificultad-quitando-callejones) | Dificultad quitando callejones | Dificultad real | Hecho |
| [F5](#f5-parámetros-propios-de-cada-algoritmo) | Parámetros por algoritmo | Controlar el aspecto | Hecho, salvo uno |
| [F6](#f6-nombres-de-algoritmo-como-texto) | Nombres como texto | Parámetros de ROS, línea de comandos | Hecho |
| [F7](#f7-todo-el-laberinto-alcanzable) | Todo alcanzable | Cuevas sin zonas muertas | Hecho |
| [F8](#f8-dimensiones-físicas) | Dimensiones físicas | Que el robot quepa | Pendiente |
| [F9](#f9-guardar-y-cargar-laberintos) | Guardar y cargar | Repetir pruebas | Pendiente |
| [F10](#f10-herramienta-de-línea-de-comandos) | Línea de comandos | Usarlo sin escribir código | Pendiente |
| [F11](#f11-semillas-reproducibles-entre-plataformas) | Semillas portables | Misma semilla en cualquier máquina | Hecho |
| [F12](#f12-generación-paso-a-paso) | Paso a paso | Animar la generación | Pendiente |
| [F13](#f13-más-algoritmos-y-formas) | Más algoritmos y formas | Variedad | Uno hecho |

Para usar el laberinto con un robot, lo siguiente son F1, F2 y F8, en ese orden.

---

## Pendientes: usar el laberinto en un robot

### F1. Convertir el laberinto en una rejilla de ocupación

**Qué.** Una función que convierta el `MazeGrid` en una imagen de celdas libres y ocupadas, que es el formato que usan los mapas de navegación.

**Por qué.** En `MazeGrid` los muros no tienen grosor: están entre celdas. Un mapa de navegación necesita píxeles ocupados. Es el paso que falta para usar el laberinto con Nav2 o con cualquier planificador.

**Cómo.**

```cpp
struct OccupancyRaster {
    int width, height;            // en píxeles
    std::vector<uint8_t> data;    // 0 = libre, 100 = ocupado
};
OccupancyRaster rasterize(const MazeGrid& grid, int cell_px, int wall_px);
```

Cada celda ocupa `cell_px` píxeles libres y cada muro `wall_px` píxeles ocupados. Los valores 0 y 100 son los de `nav_msgs/OccupancyGrid`, así que el paquete de ROS solo tiene que copiar el vector y añadir resolución y origen.

En una cueva, las celdas de roca (las que no tienen ninguna abertura) deben quedar ocupadas enteras, no solo sus muros.

Complemento natural: escribir el resultado como imagen PGM más el fichero YAML que carga `map_server`.

**Atención.** El eje `y` del laberinto crece hacia abajo y el de un mapa de ROS hacia arriba. La conversión tiene que voltearlo, y conviene un test que lo fije.

### F2. Extraer los muros como segmentos

**Qué.** Una función que devuelva la lista de muros como segmentos `(x1, y1, x2, y2)`, uniendo los tramos alineados en uno solo.

**Por qué.** Sirve para generar un mundo de Gazebo (cada segmento es una caja) y para dibujar el laberinto en RViz con marcadores. Unir tramos reduce mucho el número de piezas: una pared recta de 30 celdas es una caja, no 30.

**Cómo.** Recorrer las filas buscando rachas de muro horizontal consecutivas, y las columnas buscando rachas verticales. A partir de la lista, un exportador a SDF es una plantilla de texto.

### F8. Dimensiones físicas

**Qué.** Asociar al laberinto un tamaño de celda y un grosor de muro en metros.

**Por qué.** F1 y F2 necesitan esos datos, y permiten comprobar antes de simular si el robot cabe por los pasillos.

**Cómo.** Un `struct MazeGeometry { double cell_size; double wall_thickness; };` que se pasa a los exportadores. No hace falta guardarlo en `MazeGrid`.

Relacionado: **salidas de más de una celda de ancho**, para robots más anchos que un pasillo.

---

## Pendientes: analizar y usar con comodidad

### F3. Resolver y medir el laberinto

**Qué.** Funciones de análisis sobre un laberinto ya generado.

- **Camino más corto** entre dos celdas o entre dos salidas.
- **Mapa de distancias** desde una celda a todas las demás.
- **Par de celdas más alejado**, para colocar inicio y meta en el recorrido más largo posible.
- **Métricas**: número de callejones, de cruces, longitud de la solución.

**Por qué.**

- Da un camino de referencia con el que comparar el que encuentre el robot.
- Permite elegir inicio y meta dentro del laberinto, no solo en el borde.
- Las métricas sirven para comparar algoritmos con datos.

**Cómo.** Todo sale de una búsqueda en anchura. El par más alejado de un laberinto perfecto se obtiene con dos búsquedas: desde una celda cualquiera hasta la más lejana, y desde esa hasta la más lejana otra vez.

Los tests ya calculan regiones, callejones y pasos abiertos ([maze_test_utils.hpp](../test/maze_test_utils.hpp)); ese código es un buen punto de partida.

### F9. Guardar y cargar laberintos

**Qué.** Escribir un `MazeGrid` a un fichero y volver a leerlo.

**Por qué.** Para compartir un laberinto concreto o conservarlo aunque un cambio futuro en un algoritmo altere lo que da su semilla.

**Cómo.** Un formato de texto mínimo basta: una línea con ancho y alto, los muros de cada celda como un dígito hexadecimal y la lista de salidas. Los tests ya usan esa representación de los muros para comparar laberintos.

### F10. Herramienta de línea de comandos

**Qué.** Un ejecutable con argumentos, en lugar de la demo actual.

```
maze_gen_cli --algorithm dfs --width 20 --height 20 --seed 42 --exits 2 --format pgm --output mapa
```

**Por qué.** La demo ([mazes.cpp](../src/mazes.cpp)) solo acepta la carpeta de salida; el tamaño, la semilla y las dificultades están fijos en el código. Con argumentos sirve para generar mapas sin escribir C++.

**Cómo.** El nombre del algoritmo ya se puede leer con `algorithm_from_string`, y las opciones inválidas ya dan un mensaje claro. Faltan los formatos de salida de F1, F2 y F9.

### F12. Generación paso a paso

**Qué.** Poder observar el laberinto mientras se construye.

**Por qué.** Para animar la generación en RViz o en un terminal. Es la mejor forma de entender (y enseñar) cómo trabaja cada algoritmo.

**Cómo.** Lo menos invasivo es un aviso opcional que `MazeGrid` invoque cada vez que cambia un muro (`carve`, `add_wall`, `add_exit`): las estrategias no cambian. Quien escucha recibe `(x, y, dirección)` y decide qué dibujar.

### F13. Más algoritmos y formas

**Hecho.** [Houston](algoritmos.md#houston), que combina Aldous-Broder y Wilson.

**Pendiente.**

- **Máscaras**: marcar celdas que no forman parte del laberinto, para hacer formas no rectangulares o dejar huecos para obstáculos.
- **Laberintos de varias plantas**, con celdas que conectan niveles.

Añadir un algoritmo son tres pasos, descritos en [calidad_codigo.md](calidad_codigo.md#c5-añadir-un-algoritmo).

---

## Hechas

### F4. Dificultad quitando callejones

La opción `difficulty` (de 0 a 10) regula cuántos callejones sin salida quedan, igual en los 12 algoritmos de laberinto. Con 10 el laberinto es perfecto, con un único camino entre dos puntos; con 0 no queda ningún callejón y hay rutas alternativas por todas partes.

Cómo funciona y su efecto medido: [algoritmos.md](algoritmos.md#callejones-sin-salida).

### F5. Parámetros propios de cada algoritmo

| Algoritmo | Opción | Efecto |
|-----------|--------|--------|
| Growing Tree | `growing_tree_newest_ratio` | De un laberinto tipo Prim (0) a uno tipo DFS (1) |
| Sidewinder | `sidewinder_east_probability` | Tramos horizontales más largos o más cortos |
| Eller | `ellers_join_probability` | Más o menos uniones en horizontal |
| Generador de cuevas | `cave_level` | Cuánta roca, de forma progresiva |
| Habitaciones | `rooms`, `room_level` | Desactivarlas, o cuántas y de qué tamaño |

**Pendiente.** Binary Tree siempre sesga hacia el noroeste. Elegir la esquina (noreste, suroeste, sureste) cambiaría la dirección de su diagonal.

### F6. Nombres de algoritmo como texto

```cpp
std::string_view to_string(MazeAlgorithm algorithm);                     // "hunt_and_kill"
std::optional<MazeAlgorithm> algorithm_from_string(std::string_view name);
std::span<const MazeAlgorithm> all_algorithms();
```

Los nombres van en minúsculas con guiones bajos. `algorithm_from_string` exige el nombre exacto y devuelve vacío si no lo reconoce, que es lo que conviene para validar un parámetro de ROS.

### F7. Todo el laberinto alcanzable

Tras `generate()`, desde cualquier salida se llega a todas las celdas abiertas. En los laberintos lo garantiza el propio algoritmo; en las cuevas, un paso final une lo que haya quedado suelto rompiendo el mínimo de muros ([algoritmos.md](algoritmos.md#conexión)).

### F11. Semillas reproducibles entre plataformas

La misma semilla da el mismo laberinto en cualquier compilador, y cada fase de la generación usa su propia secuencia de números aleatorios ([algoritmos.md](algoritmos.md#números-aleatorios-y-semillas)).

**Pendiente.** Comprobarlo fuera de GCC. Basta con pasar los tests de `test_reproducibility.cpp` en Clang o MSVC.
