# Documentación de `maze_gen`

Cómo funcionan los algoritmos del paquete, cómo se usa, qué fallos tenía y cómo se corrigieron, cuánto tarda y qué queda por hacer. Estado a 13 de octubre de 2026.

| Documento | Contenido |
|-----------|-----------|
| [algoritmos.md](algoritmos.md) | Modelo de datos, fases de `generate()` y explicación de los 13 algoritmos |
| [errores.md](errores.md) | Los 12 fallos de corrección encontrados, todos corregidos |
| [optimizacion.md](optimizacion.md) | Tiempos medidos y qué se hizo para llegar a ellos |
| [calidad_codigo.md](calidad_codigo.md) | Las 13 mejoras de código: qué se hizo en cada una y lo poco que queda |
| [nuevas_features.md](nuevas_features.md) | Funciones nuevas: las ya incorporadas y las pendientes para usarlo con ROS 2 |

## Cómo se usa

En el `package.xml` del paquete que lo use:

```xml
<depend>maze_gen</depend>
```

En su `CMakeLists.txt`:

```cmake
find_package(maze_gen REQUIRED)
target_link_libraries(mi_nodo PRIVATE maze_gen::maze_gen)
```

En el código:

```cpp
#include "maze_gen/maze_generator.hpp"

maze_gen::MazeStrategyOptions options;
options.seed = 42;
options.num_random_exits = 2;

maze_gen::MazeGenerator generator{maze_gen::MazeAlgorithm::Kruskal};
maze_gen::MazeGrid grid = generator.generate(20, 15, options);

for (const maze_gen::Exit & exit : grid.exits()) { /* exit.x, exit.y, exit.dir */ }
bool wall = grid.has_wall(3, 4, maze_gen::Dir::N);
```

El algoritmo también se puede elegir por nombre, que es lo que hace falta para un parámetro de ROS:

```cpp
auto algorithm = maze_gen::algorithm_from_string("hunt_and_kill");  // std::optional
```

Las opciones están documentadas una a una en [maze_strategy_options.hpp](../include/maze_gen/maze_strategy_options.hpp). Un valor fuera de rango lanza `std::invalid_argument` con un mensaje que dice cuál y por qué.

## Compilar y probar

Desde la raíz del espacio de trabajo:

```bash
colcon build --packages-select maze_gen
colcon test --packages-select maze_gen && colcon test-result --verbose
```

Para pasar los tests con detección de fallos de memoria, en una carpeta de compilación aparte:

```bash
colcon build --packages-select maze_gen --build-base build_asan --install-base install_asan \
  --cmake-args -DMAZE_GEN_SANITIZE=ON
colcon test --packages-select maze_gen --build-base build_asan --install-base install_asan
```

La demo escribe un fichero por algoritmo, con el laberinto a tres dificultades:

```bash
ros2 run maze_gen maze_gen_demo [carpeta_de_salida]   # por defecto ./data
```

## Estado en resumen

**Los 13 algoritmos funcionan.** Los 12 de laberinto generan laberintos perfectos en todas las pruebas, y el generador de cuevas produce cuevas de una sola pieza.

**Hay tests.** 1115 casos en 7 ficheros, más una comprobación de formato. Pasan también con AddressSanitizer y UBSan. Para comprobar que detectan fallos se reintrodujeron 30 errores uno a uno en una copia del código: cazaron 29, y el que no cazaron resultó ser código que sobraba y se eliminó.

**Es rápido.** Todos los algoritmos menos Aldous-Broder generan un laberinto de 400×400 en menos de 27 ms, con todas las fases incluidas.

**Una semilla da el mismo laberinto en cualquier compilador.** Los números aleatorios ya no dependen de las partes de la biblioteca estándar que cambian de una plataforma a otra. Solo se ha podido ejecutar con GCC; los tests de [test_reproducibility.cpp](../test/test_reproducibility.cpp) fijan valores exactos y son la comprobación que hay que pasar en un compilador nuevo.

**La interfaz pública son cinco cabeceras.** Todo lo demás es interno y no se instala.

## Qué cambia respecto a la versión anterior

Esta revisión cambia la interfaz. Lo que hay que saber si ya se usaba el paquete:

| Antes | Ahora |
|-------|-------|
| `getExits()` devolvía tuplas | `exits()` devuelve `Exit {x, y, dir}` |
| `hasWall`, `addWall`, `displaySimple`, `setMazeStrategy`... | `has_wall`, `add_wall`, `display_simple`, `set_algorithm`... |
| `openBorder` más `addExit` | Una sola llamada: `add_exit` |
| 8 direcciones, con diagonales | 4 direcciones. Ya no hay pasos en diagonal |
| `difficulty` solo afectaba a cuevas y habitaciones | `difficulty` regula los callejones sin salida en todos los laberintos; por defecto vale 10 |
| Dificultad de la cueva y de las habitaciones | `cave_level` y `room_level`, cada una con su opción |
| Valores fuera de rango se recortaban o se ignoraban | Lanzan `std::invalid_argument` |
| 27 cabeceras instaladas | 5 |
| 12 algoritmos | 13: se añade Houston |

**Todas las semillas dan ahora un laberinto distinto al de antes**, en los 13 algoritmos, porque ha cambiado la forma de generar los números aleatorios. A cambio, a partir de aquí son estables entre plataformas.

## Qué queda por hacer

1. **Decidir la licencia.** `package.xml` sigue con `TODO: License declaration`. Es una decisión tuya, no técnica.
2. **Lo necesario para usar el laberinto en ROS**: convertirlo en rejilla de ocupación ([F1](nuevas_features.md#f1-convertir-el-laberinto-en-una-rejilla-de-ocupación)) y extraer los muros como segmentos ([F2](nuevas_features.md#f2-extraer-los-muros-como-segmentos)).
3. **Probar la reproducibilidad en otro compilador** (Clang o MSVC) pasando los tests.

## Cómo se ha medido

Las cifras de estos documentos salen de programas de medida escritos para la revisión, que no forman parte del repositorio. Las comprobaciones de corrección sí están en el paquete, como tests.

- **Estructura.** Rejillas de 30×30, semillas 1 a 20. Para cada laberinto se cuentan las regiones conectadas, los pasos abiertos, los huecos en el borde y las celdas con una sola abertura.
- **"Solo el algoritmo"** significa llamar a la estrategia directamente sobre una rejilla nueva. **"`generate()` completo"** usa las opciones por defecto con 4 salidas aleatorias.
- **Tiempos.** `g++ 13.3 -O2`, mejor de 5 ejecuciones, en un AMD Ryzen 5 PRO 4650G bajo WSL2. Sirven para comparar entre sí, no como valores absolutos. De un día a otro varían en torno a un 5 %.
- **Fallos de memoria.** Los tests completos compilados con `-fsanitize=address,undefined -D_GLIBCXX_ASSERTIONS` y con los `assert` de la librería activos.
- **"Antes"** es el commit `26a8a2e`.
