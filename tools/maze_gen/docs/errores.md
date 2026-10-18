# Errores encontrados y corregidos

Los 12 fallos de corrección que tenía `maze_gen` están arreglados, y cada uno tiene tests que fallan si vuelve a aparecer. Este documento deja constancia de cada uno: qué pasaba, cuál era la causa, qué se cambió y cómo se comprobó.

"Antes" es el commit `26a8a2e`. Las cifras de "antes" son medias sobre 20 semillas en 30×30. "Componentes" son grupos de celdas conectadas entre sí; un laberinto correcto tiene exactamente 1.

| Id | Qué pasaba | Antes | Ahora |
|----|-----------|-------|-------|
| [E1](#e1-acceso-a-memoria-liberada-en-el-resolver) | El resolver leía memoria liberada | Segfault a 200×200 | Sin errores con sanitizers |
| [E2](#e2-hunt-and-kill-partido-en-trozos) | Hunt-and-Kill partido en trozos | 62,1 componentes | 1 |
| [E3](#e3-recursive-division-con-la-lógica-invertida) | Recursive Division invertido | 134,1 componentes | 1 |
| [E4](#e4-binary-tree-abría-el-borde-exterior) | Binary Tree abría el borde | 29,8 huecos, 21,8 componentes | 0 huecos, 1 componente |
| [E5](#e5-ciclos-en-wilson) | Wilson generaba ciclos | 908,0 pasos | 899,0 (perfecto) |
| [E6](#e6-zonas-aisladas-tras-generate) | Zonas aisladas tras `generate()` | Hasta 114 componentes | 1 en los 13 algoritmos |
| [E7](#e7-salidas-acumuladas-al-reutilizar-la-rejilla) | Salidas acumuladas | 2, 4, 6 | 2, 2, 2 |
| [E8](#e8-rango-inválido-en-las-habitaciones) | Rango inválido en habitaciones | Aserción en laberintos pequeños | Sin errores |
| [E9](#e9-growing-tree-se-colgaba-con-11) | Growing Tree colgado en 1×1 | Bucle infinito | Termina |
| [E10](#e10-tamaños-inválidos-sin-validar) | Tamaños 0×0 o negativos | Comportamiento indefinido | Excepción |
| [E11](#e11-conectividad-de-la-cueva) | Cueva con grupos sueltos | De 1,6 a 2,4 componentes | 1 |
| [E12](#e12-salidas-fijas-inválidas-se-daban-por-buenas) | Salidas fijas inválidas aceptadas | Índices fuera de la rejilla | Excepción |

Al final hay una sección sobre las [limitaciones](#limitaciones) que se señalaron en la revisión, casi todas ya resueltas.

---

## E1. Acceso a memoria liberada en el resolver

**Qué pasaba.** Cuando las salidas no estaban conectadas, la búsqueda de camino del resolver leía un nodo a través de una referencia que ya no era válida. Con AddressSanitizer saltaba `heap-use-after-free`, y compilado normal con `-O2` el programa moría con `SIGSEGV` a 200×200. A tamaños pequeños pasaba desapercibido.

**Causa.** Se guardaba una referencia a un elemento de un vector y dentro del bucle se añadían elementos a ese mismo vector. Al realojarse, la referencia quedaba apuntando a memoria liberada.

**Arreglo.** El resolver se reescribió entero y ya no usa esa búsqueda. Hoy es [region_connector.cpp](../src/region_connector.cpp); su algoritmo se describe en [algoritmos.md](algoritmos.md#conexión).

**Comprobación.** Los 1115 tests pasan con AddressSanitizer y UBSan sin ningún aviso.

---

## E2. Hunt-and-Kill partido en trozos

**Qué pasaba.** El laberinto quedaba dividido en zonas sin comunicación: 62,1 componentes y 23,4 celdas encerradas de media.

**Causa.** La fase de "caza" tomaba la primera celda sin visitar y seguía desde ella sin abrir ningún muro hacia la parte ya construida. Cada caza empezaba un trozo independiente.

**Arreglo.** En [hunt_and_kill_strategy.cpp](../src/hunt_and_kill_strategy.cpp) la caza solo acepta una celda sin visitar que tenga algún vecino visitado, y abre el muro hacia uno de ellos elegido al azar.

**Comprobación.** Laberinto perfecto en todas las pruebas. Su 9,8 % de callejones confirma que se comporta como debe: casi igual que DFS (10,2 %).

---

## E3. Recursive Division con la lógica invertida

**Qué pasaba.** El resultado no era un laberinto: 134,1 componentes y 32,6 celdas encerradas de media.

**Causa.** El algoritmo clásico parte de un espacio vacío y añade muros. El código partía de una rejilla con todos los muros y, donde debía levantar uno, abría pasos en toda la línea menos en una celda.

**Arreglo.** [recursive_division_strategy.cpp](../src/recursive_division_strategy.cpp) está reescrito:

1. Abre todos los muros interiores para partir de una sala vacía.
2. Divide la zona con un muro que tiene un único paso, cortando por el lado más largo.
3. Repite en las dos mitades hasta que una zona tenga una celda de ancho.

Para esto `MazeGrid` tiene una operación que antes no existía, `add_wall`.

**Efecto colateral.** Como este algoritmo reconstruye todos los muros, borraría las habitaciones abiertas antes que él. Por eso `generate()` abre las habitaciones **después** del algoritmo.

**Comprobación.** Laberinto perfecto en todas las pruebas, y un test específico de que las habitaciones sobreviven.

---

## E4. Binary Tree abría el borde exterior

**Qué pasaba.** El muro exterior quedaba con 29,8 huecos de media y parte del laberinto no era alcanzable (21,8 componentes).

**Causa.** Se elegía norte u oeste al azar para todas las celdas, incluidas las de la fila superior y la columna izquierda, donde una de las dos direcciones sale de la rejilla.

**Arreglo.** Dos cambios:

- [binary_tree_strategy.cpp](../src/binary_tree_strategy.cpp) trata los bordes aparte: en la fila superior solo se va al oeste, en la columna izquierda solo al norte, y la esquina no abre nada.
- `MazeGrid::carve` ya no toca nada si el vecino está fuera de la rejilla. El borde exterior solo lo abre `add_exit`. Así un despiste en una estrategia no puede agujerear el borde.

**Comprobación.** Laberinto perfecto y 0 huecos en el borde en los 13 algoritmos.

---

## E5. Ciclos en Wilson

**Qué pasaba.** El laberinto salía conectado pero con pasos de más: 908,0 de media cuando uno perfecto tiene 899. Dejaba de ser un árbol generador uniforme.

**Causa.** Al borrar un bucle del paseo aleatorio se conservaba en el camino la entrada de la celda donde se cerraba el bucle, con su dirección antigua. Luego esa celda se añadía otra vez con la dirección nueva y se abrían los dos muros.

**Arreglo.** En [wilsons_strategy.cpp](../src/wilsons_strategy.cpp) se descarta también esa entrada, de modo que la celda se vuelve a añadir limpia.

**Comprobación.** Exactamente 899 pasos en las 20 semillas. Un test compara además su porcentaje de callejones con el de Aldous-Broder: tienen que coincidir, porque los dos producen la misma distribución.

---

## E6. Zonas aisladas tras `generate()`

**Qué pasaba.** `generate()` terminaba comprobando que las salidas se alcanzaban entre sí, no que todo el laberinto fuera alcanzable. Si el algoritmo dejaba zonas aisladas, se entregaban así. Con una sola salida no se comprobaba nada.

**Arreglo.** Dos partes:

- Los cuatro algoritmos que dejaban zonas aisladas están corregidos (E2, E3, E4, E11), así que un laberinto es siempre una sola región.
- En las cuevas, donde una salida puede caer en roca, `generate()` termina uniendo todas las zonas abiertas y todas las salidas ([algoritmos.md](algoritmos.md#conexión)).

**Comprobación.** Tras `generate()`, 1 región en los 13 algoritmos, con 0, 1 y 4 salidas.

---

## E7. Salidas acumuladas al reutilizar la rejilla

**Qué pasaba.** Al llamar a `generate()` varias veces sobre el mismo `MazeGrid`, la lista de salidas incluía las de generaciones anteriores, y el resolver abría caminos hacia ellas.

**Causa.** `MazeGrid::reset` reiniciaba las celdas pero no la lista de salidas.

**Arreglo.** `reset` vacía también las salidas ([maze_grid.cpp](../src/maze_grid.cpp)).

**Comprobación.** Cuatro llamadas con 3 salidas sobre la misma rejilla: siempre 3 salidas y 3 huecos en el borde. Reutilizar una rejilla da exactamente el mismo laberinto que una nueva.

---

## E8. Rango inválido en las habitaciones

**Qué pasaba.** En laberintos pequeños se construía una distribución con el máximo menor que el mínimo, que es comportamiento indefinido.

**Arreglo.** [room_generator.cpp](../src/room_generator.cpp) calcula el lado máximo que cabe (`min(ancho, alto) - 3`). Si es menor que el tamaño mínimo de habitación no genera ninguna; si no, limita el tamaño máximo a ese valor. La posición se sortea directamente dentro del margen válido.

**Comprobación.** Tamaños de 5×5 a 12×12 con los 11 niveles y 5 semillas, y laberintos de 1×1 a 4×4, sin ninguna aserción.

---

## E9. Growing Tree se colgaba con 1×1

**Qué pasaba.** Bucle infinito en una rejilla de una sola celda.

**Causa.** La celda activa solo se retiraba de la lista dentro del bucle de vecinos. Con 0 vecinos el bucle no se ejecutaba y la lista nunca se vaciaba.

**Arreglo.** En [growing_tree_strategy.cpp](../src/growing_tree_strategy.cpp) la celda se retira en cuanto no le quedan vecinos sin visitar, tenga los que tenga.

**Comprobación.** Los 12 algoritmos de laberinto terminan y dan un laberinto perfecto en 1×1, 1×2, 2×1, 1×9 y 9×1.

---

## E10. Tamaños inválidos sin validar

**Qué pasaba.** `MazeGrid(0, 0)` se construía sin error y varias estrategias hacían algo indefinido con ella, incluida una división por cero. Con tamaños negativos se pedía una cantidad absurda de memoria.

**Arreglo.** El constructor de `MazeGrid` lanza `std::invalid_argument` si el ancho o el alto son menores que 1, o si su producto no cabe en un `int`.

**Comprobación.**

| Tamaño | Resultado |
|--------|-----------|
| 0×0, 0×5, -3×4 | `MazeGrid: width and height must be at least 1` |
| 70000×70000 | `MazeGrid: width * height is too large` |

---

## E11. Conectividad de la cueva

**Qué pasaba.** Podían quedar grupos de celdas abiertas pegados al borde sin conexión con el resto: de 1,6 a 2,4 componentes de media según la dificultad.

**Causa.** La búsqueda de grupos solo empezaba desde celdas interiores, así que un grupo que estuviera entero en el borde nunca se detectaba.

**Arreglo.** [cellular_automaton_strategy.cpp](../src/cellular_automaton_strategy.cpp) recorre toda la rejilla al buscar grupos.

**Comprobación.** 1 región en los 11 niveles, con 15 semillas cada uno.

---

## E12. Salidas fijas inválidas se daban por buenas

Este fallo no estaba en la revisión inicial: lo encontraron los tests al escribir los casos de entradas incorrectas.

**Qué pasaba.** Tres tipos de salida fija incorrecta se aceptaban:

- **Fuera de la rejilla.** Una salida en `(-1, 0)` o en `(10, 9)` sobre una rejilla de 10×10 pasaba la validación si su otra coordenada caía en un borde. No se abría ningún muro, pero la salida se guardaba, y el resolver usaba después sus coordenadas como índice, leyendo fuera de la memoria de la rejilla.
- **En diagonal.** Se aceptaban direcciones diagonales en las esquinas.
- **Repetidas.** La misma salida podía aparecer varias veces en la lista, o coincidir con una aleatoria.

**Causa.** La comprobación de "está en el perímetro" miraba cada coordenada por separado, y la salida se apuntaba aunque no se hubiera abierto.

**Arreglo.**

- Abrir el muro y apuntar la salida son ahora una sola operación, `MazeGrid::add_exit`, que no apunta nada si no ha abierto el muro.
- `generate()` valida todas las salidas fijas antes de empezar y lanza `std::invalid_argument` indicando cuál falla y por qué.
- Ya no existen direcciones diagonales.
- Las salidas aleatorias evitan los huecos que ocupa una fija.

**Comprobación.** Tests con 8 posiciones fuera de la rejilla, direcciones hacia dentro, valores de dirección inventados y repeticiones: todas se rechazan y la rejilla queda intacta.

---

## Limitaciones

La revisión señaló cinco limitaciones que no eran fallos. Cuatro están resueltas.

**La misma semilla no daba el mismo laberinto en otra plataforma. Resuelto.** Los números aleatorios ya no usan `std::shuffle` ni las distribuciones de la biblioteca estándar, que es lo que variaba de un compilador a otro. Detalle en [algoritmos.md](algoritmos.md#números-aleatorios-y-semillas). Queda una salvedad: solo se ha podido ejecutar con GCC. Los tests de reproducibilidad fijan valores exactos y son lo que hay que pasar para confirmarlo en Clang o MSVC.

**Habitaciones, algoritmo y salidas usaban la misma secuencia aleatoria. Resuelto.** Cada fase tiene ahora su propia secuencia, derivada de la misma semilla.

**`UnionFind::reset` no reiniciaba los rangos. Resuelto.** Nadie llamaba a esa función; se ha eliminado.

**Los pasos en diagonal de la cueva. Resuelto.** Para un robot con tamaño físico, pasar entre dos celdas que solo se tocan por la esquina no es posible. Ya no existen: la cueva solo conecta celdas que comparten un lado, y las direcciones diagonales han desaparecido de toda la librería.

**Las semillas antiguas dan laberintos distintos. Sigue siendo así, y ahora en los 13 algoritmos.** Es consecuencia de haber cambiado la forma de generar los números aleatorios para que sean portables. No tiene arreglo, pero no debería volver a pasar: los tests de reproducibilidad avisan si un cambio altera el laberinto de una semilla.
