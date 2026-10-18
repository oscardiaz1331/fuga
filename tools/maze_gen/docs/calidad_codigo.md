# Mejoras de código

Las 13 mejoras de código que señaló la revisión, con lo que se hizo en cada una. Doce están hechas; queda una decisión que no es técnica (la licencia) y algún detalle menor que se indica en su apartado.

| Id | Tema | Estado |
|----|------|--------|
| [C1](#c1-tests) | Tests | Hecho |
| [C2](#c2-validación-de-entradas) | Validación de entradas | Hecho |
| [C3](#c3-avisos-del-compilador) | Avisos del compilador | Hecho |
| [C4](#c4-modelo-de-celda-duplicado) | Modelo de celda duplicado | Hecho |
| [C5](#c5-añadir-un-algoritmo) | Añadir un algoritmo | Hecho |
| [C6](#c6-qué-significa-la-dificultad) | Qué significa la dificultad | Hecho |
| [C7](#c7-interfaz-pública) | Interfaz pública | Hecho |
| [C8](#c8-cabeceras-internas-fuera-de-la-interfaz) | Cabeceras internas fuera de la interfaz | Hecho |
| [C9](#c9-código-muerto) | Código muerto | Hecho |
| [C10](#c10-cabeceras-e-includes) | Cabeceras e includes | Hecho |
| [C11](#c11-pequeñas-incoherencias) | Pequeñas incoherencias | Hecho |
| [C12](#c12-formato-y-estilo) | Formato y estilo | Hecho |
| [C13](#c13-empaquetado) | Empaquetado | Falta la licencia |

---

## C1. Tests

Era la carencia más importante: cuatro algoritmos no generaban un laberinto correcto y nada lo detectaba.

**Qué hay.** 1115 casos con `ament_cmake_gtest`, en [test/](../test/):

| Fichero | Casos | Qué prueba |
|---------|------:|------------|
| `test_maze_algorithms.cpp` | 741 | Cada algoritmo por separado: laberinto perfecto en 13 tamaños y 6 semillas, borde cerrado, muros coherentes, semillas, forma característica, opciones propias y regresiones |
| `test_maze_generator.cpp` | 245 | `generate()` completo: salidas, dificultad, habitaciones, reutilización, y todas las opciones que deben rechazarse |
| `test_support_types.cpp` | 44 | Números aleatorios, nombres de algoritmo, `Room`, `UnionFind` |
| `test_maze_grid.cpp` | 35 | La rejilla: construcción, muros, vecinos, salidas, reinicio, dibujo |
| `test_exits_and_rooms.cpp` | 24 | Salidas, habitaciones y eliminación de callejones, sin el resto |
| `test_region_connector.cpp` | 16 | La unión de regiones, sobre rejillas hechas a mano con respuesta conocida |
| `test_reproducibility.cpp` | 10 | Valores exactos para una semilla: números aleatorios, cada algoritmo y el proceso completo |

Cubren casos buenos y casos malos: tamaños degenerados (1×1, 1×N, N×1, 2×2), entradas fuera de rango, coordenadas fuera de la rejilla, valores no numéricos, listas vacías y repetidas.

**Cómo se sabe que sirven.** Se reintrodujeron 30 errores, uno a uno, en una copia del código: los fallos originales de la revisión y otros nuevos en cada pieza. Los tests cazaron 29. El que no cazaron no era un error: reveló que el conector de regiones tenía una parte que nunca hacía falta, y se eliminó.

**Con sanitizers.** La opción `-DMAZE_GEN_SANITIZE=ON` compila la librería y los tests con AddressSanitizer, UBSan, las comprobaciones de la biblioteca estándar y los `assert` propios activos. Los 1115 casos pasan sin ningún aviso. Las órdenes están en el [README](README.md#compilar-y-probar).

**Encontraron un fallo real.** Al escribir los casos de salidas fijas incorrectas apareció [E12](errores.md#e12-salidas-fijas-inválidas-se-daban-por-buenas), que no estaba en la revisión.

---

## C2. Validación de entradas

Antes, un valor fuera de rango se recortaba o se ignoraba sin avisar. Ahora `MazeGenerator::generate` valida todo antes de tocar la rejilla y lanza `std::invalid_argument` con un mensaje que nombra la opción y el valor:

```
maze_gen: difficulty must be between 0 and 10, got 99
maze_gen: fixed exit #2 at (0, 3) is repeated
maze_gen: num_random_exits is 13 but this grid only has room for 12
```

Se comprueban los rangos de `difficulty`, `room_level` y `cave_level`, las tres probabilidades (incluido el caso de un valor no numérico), cada salida fija y el número de salidas aleatorias. Una llamada rechazada deja la rejilla exactamente como estaba.

`MazeGrid` rechaza los tamaños inválidos en su constructor, y un valor de `MazeAlgorithm` que no existe se rechaza al crear el generador.

Las funciones de `MazeGrid` que modifican muros devuelven `false` si se les pasa una celda de fuera. Las de consulta (`walls`, `has_wall`) exigen una celda válida y lo comprueban con `assert`, que solo está activo en compilaciones de depuración o con sanitizers.

---

## C3. Avisos del compilador

[CMakeLists.txt](../CMakeLists.txt) activa `-Wall -Wextra -Wpedantic` para la librería, la demo y los tests. Todo compila sin ningún aviso.

---

## C4. Modelo de celda duplicado

El modelo de celda tenía varias piezas repetidas: las direcciones definidas dos veces, la lectura de muros como método y como función libre, tres tipos de celda dentro de un `std::variant` y además un campo con la misma información.

La rejilla guarda ahora un byte de muros por celda y [direction.hpp](../include/maze_gen/direction.hpp) solo define las cuatro direcciones.

---

## C5. Añadir un algoritmo

Antes había que tocar cuatro sitios, y olvidar uno provocaba un fallo en ejecución. La fábrica era además un objeto global que se modificaba sin protección entre hilos.

Ahora son tres pasos:

1. Un valor en el enumerado de [maze_algorithm.hpp](../include/maze_gen/maze_algorithm.hpp).
2. Una clase en [strategies.hpp](../src/strategies.hpp), con su `.cpp`.
3. Una fila en la tabla de [algorithm_registry.cpp](../src/algorithm_registry.cpp), con el nombre y si genera laberintos perfectos.

La tabla es constante: no hay registro en ejecución ni estado global. De ella salen `to_string`, `algorithm_from_string` y `all_algorithms`.

Si se olvida la fila de la tabla, el algoritmo no existe para la librería: crear un generador con él lanza una excepción que lo dice, en vez de fallar más tarde de forma rara.

Los tests recorren esa tabla, así que un algoritmo nuevo queda probado sin escribir nada más. Solo hay que añadir su laberinto de referencia en `test_reproducibility.cpp`, y un test avisa si se olvida.

---

## C6. Qué significa la dificultad

`difficulty` solo afectaba al generador de cuevas, donde no era progresiva, y a las habitaciones, donde más dificultad hacía el laberinto más fácil. Los algoritmos de laberinto la ignoraban.

Se han separado los tres conceptos:

| Opción | Rango | Qué controla |
|--------|-------|--------------|
| `difficulty` | 0 a 10 | Cuántos callejones sin salida quedan, igual en los 12 algoritmos de laberinto. Con 10 es un laberinto perfecto; con 0 no queda ninguno |
| `room_level` | 0 a 10 | Cuántas habitaciones y de qué tamaño. `rooms = false` las desactiva |
| `cave_level` | 0 a 10 | Cuánta roca tiene la cueva, del 16 % al 52 % de las celdas, de forma progresiva |

Y hay opciones propias para los algoritmos que tenían un parámetro fijo en el código: `growing_tree_newest_ratio`, `sidewinder_east_probability` y `ellers_join_probability`.

Las tablas con el efecto medido de cada opción están en [algoritmos.md](algoritmos.md#fases-comunes).

**Valor por defecto.** `difficulty` vale ahora 10 por defecto, que es lo que daba antes cualquier valor: un laberinto perfecto.

---

## C7. Interfaz pública

- **Las salidas son un tipo con nombre.** `Exit {x, y, dir}` en vez de una tupla.
- **`generate` puede devolver la rejilla.** Hay dos formas: `generate(grid, options)` rellena una existente y `generate(ancho, alto, options)` devuelve una nueva.
- **Lectura y escritura separadas.** Las consultas de `MazeGrid` son métodos `const`.
- **El resolver se llama por lo que hace.** `RegionConnector`, y ya no es parte de la interfaz pública.
- **Abrir una salida es una sola operación.** `add_exit` abre el muro y la apunta; antes eran dos llamadas y se podía hacer una sin la otra.

---

## C8. Cabeceras internas fuera de la interfaz

Se instalaban 27 cabeceras, y un paquete que usara la librería veía todas sus piezas internas.

Ahora la interfaz pública son las 5 de [include/maze_gen/](../include/maze_gen/):

| Cabecera | Contenido |
|----------|-----------|
| `maze_generator.hpp` | `MazeGenerator`. Incluye las otras cuatro |
| `maze_grid.hpp` | `MazeGrid`, `Exit`, `Neighbor` |
| `maze_strategy_options.hpp` | `MazeStrategyOptions` |
| `maze_algorithm.hpp` | `MazeAlgorithm` y sus nombres |
| `direction.hpp` | `Dir` |

Las 11 cabeceras internas están en [src/](../src/) y no se instalan. `MazeGenerator` guarda sus piezas detrás de un puntero a una clase interna, de modo que su cabecera no necesita incluirlas.

Los tests sí ven las piezas internas, para poder probarlas una a una.

---

## C9. Código muerto

Eliminado todo lo que no se usaba: las funciones de `Room`, `UnionFind` y la rejilla que nadie llamaba, las tres funciones del resolver que `generate()` ya no necesitaba, una constante que dejaba ramas sin ejecutar en el generador de salidas, una comprobación que nunca se cumplía en Kruskal, los valores por defecto de la cueva que siempre se sobrescribían y la fábrica con su registro.

La comprobación por mutaciones encontró otro caso: el conector de regiones usaba una búsqueda con dos costes cuando una búsqueda en anchura normal da el mismo resultado. Se simplificó.

---

## C10. Cabeceras e includes

Cada fichero incluye lo que usa, en este orden: su propia cabecera, la biblioteca estándar y las cabeceras del proyecto.

`maze_grid.hpp` ya no arrastra `<iostream>` a quien la incluya: usa `<iosfwd>`.

---

## C11. Pequeñas incoherencias

Las estrategias ya no tienen estado propio. Reciben la rejilla, el generador de números aleatorios y las opciones como argumentos, todas igual. Eso resuelve de una vez varias incoherencias: unas guardaban el generador aleatorio como miembro y otras no, unas tenían el método público y otras privado, unas lo marcaban como sustituido y otras no.

También: los tipos auxiliares de cada `.cpp` están en un espacio de nombres anónimo, ningún algoritmo usa `rng() % n`, las variables se llaman por lo que contienen y `MazeGenerator` ya no tiene un destructor virtual sin ser clase base.

---

## C12. Formato y estilo

**Formato.** El de ROS 2, aplicado con `ament_uncrustify`. Se comprueba como un test más, así que un fichero mal formateado hace fallar `colcon test`. Para reformatear:

```bash
ament_uncrustify --reformat include src test
```

**Finales de línea.** Todos los ficheros usan LF, y un `.gitattributes` en la raíz del repositorio lo mantiene así en cualquier sistema.

**Nombres.** Un solo criterio, el de la guía de ROS 2: tipos en `CamelCase`, funciones y variables en `snake_case`, miembros con guion bajo al final (`width_`), constantes con `k` delante.

**Qué no se ha hecho.** No se usa `ament_cpplint`, que exige una cabecera de licencia en cada fichero y guardas de inclusión con nombre en vez de `#pragma once`. Tiene sentido activarlo cuando la licencia esté decidida.

---

## C13. Empaquetado

**Hecho.**

- El paquete se compila con optimización por defecto.
- Las fuentes están listadas una a una en `CMakeLists.txt`.
- La demo crea la carpeta de salida si no existe y acepta otra como argumento.
- Las dependencias de test están declaradas en `package.xml`.

**Pendiente: la licencia.** `package.xml` sigue con `TODO: License declaration`. Hay que elegir una (Apache-2.0 es la habitual en ROS 2, MIT y BSD-3-Clause también son comunes), ponerla en `package.xml` y añadir el fichero `LICENSE`.
