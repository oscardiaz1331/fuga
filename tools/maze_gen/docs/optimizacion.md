# Rendimiento

Cuánto tarda `maze_gen`, cuánto tardaba y qué se hizo para llegar hasta aquí.

"Antes" es el commit `26a8a2e`. Tiempos en milisegundos, compilado con `-O2`, mejor de 5 ejecuciones; condiciones en el [README](README.md#cómo-se-ha-medido).

## Antes y ahora

**Solo el algoritmo:**

| Algoritmo | 200×200 antes | 200×200 ahora | 400×400 antes | 400×400 ahora | Cambio a 400×400 |
|-----------|--------------:|--------------:|--------------:|--------------:|------------------|
| Hunt-and-Kill | 106,11 | 1,82 | 1683,28 | 8,73 | 193 veces más rápido |
| Growing Tree | 132,29 | 2,87 | 1057,86 | 11,75 | 90 veces |
| Generador de cuevas | 51,64 | 3,95 | 479,73 | 17,73 | 27 veces |
| Wilson | 29,35 | 5,26 | 436,14 | 26,84 | 16 veces |
| Aldous-Broder | 66,68 | 21,76 | 359,85 | 129,20 | 2,8 veces |
| DFS | 5,69 | 2,14 | 23,33 | 8,52 | 2,7 veces |
| Prim | 5,19 | 3,24 | 28,14 | 13,23 | 2,1 veces |
| Kruskal | 3,81 | 2,89 | 18,48 | 14,56 | 1,3 veces |
| Sidewinder | 0,73 | 0,62 | 2,76 | 2,46 | 1,1 veces |
| Binary Tree | 0,53 | 0,54 | 2,03 | 2,12 | Igual |
| Eller | 2,55 | 2,64 | 10,55 | 11,43 | Un 8 % más lento ¹ |
| Recursive Division | 0,54 | 0,92 | 2,14 | 3,66 | 1,7 veces más lento ² |
| Houston | n/a | 3,14 | n/a | 15,16 | Nuevo |

¹ Es el coste de los números aleatorios portables: ver [más abajo](#el-coste-de-los-números-aleatorios-portables).
² Antes no hacía el trabajo: generaba un resultado incorrecto ([E3](errores.md#e3-recursive-division-con-la-lógica-invertida)).

**`generate()` completo** con 4 salidas y el resto de opciones por defecto:

| Algoritmo | 200×200 antes | 200×200 ahora | 400×400 antes | 400×400 ahora |
|-----------|--------------:|--------------:|--------------:|--------------:|
| Hunt-and-Kill | segfault | 1,92 | segfault | 9,17 |
| Recursive Division | segfault | 1,07 | segfault | 4,10 |
| Growing Tree | 135,30 | 3,05 | 1072,60 | 12,21 |
| Generador de cuevas | 52,22 | 5,25 | 505,31 | 25,61 |
| Wilson | 33,34 | 5,04 | 458,73 | 26,38 |
| Binary Tree | 22,23 | 0,68 | 44,54 | 2,55 |
| Sidewinder | 4,45 | 0,77 | 19,03 | 2,90 |
| DFS | 10,10 | 2,20 | 47,76 | 8,26 |
| Prim | 8,77 | 3,42 | 43,83 | 13,67 |
| Kruskal | 7,55 | 2,90 | 46,05 | 14,19 |
| Eller | 6,49 | 2,74 | 32,37 | 11,22 |
| Aldous-Broder | 72,45 | 21,79 | 395,65 | 123,74 |
| Houston | n/a | 3,11 | n/a | 15,08 |

Todos los algoritmos menos Aldous-Broder generan ahora un laberinto de 400×400 (160 000 celdas) en menos de 27 ms.

`generate()` completo ya cuesta casi lo mismo que el algoritmo solo. Antes las fases comunes añadían 3,7 ms a 200×200, más que algunos algoritmos enteros; ahora añaden unos 0,15 ms.

## Tiempos actuales por tamaño

**Solo el algoritmo:**

| Algoritmo | 50×50 | 100×100 | 200×200 | 400×400 |
|-----------|------:|--------:|--------:|--------:|
| Binary Tree | 0,05 | 0,15 | 0,54 | 2,12 |
| Sidewinder | 0,06 | 0,17 | 0,62 | 2,46 |
| Recursive Division | 0,08 | 0,25 | 0,92 | 3,66 |
| DFS | 0,15 | 0,54 | 2,14 | 8,52 |
| Hunt-and-Kill | 0,12 | 0,43 | 1,82 | 8,73 |
| Eller | 0,17 | 0,63 | 2,64 | 11,43 |
| Growing Tree | 0,23 | 0,73 | 2,87 | 11,75 |
| Prim | 0,22 | 0,82 | 3,24 | 13,23 |
| Kruskal | 0,15 | 0,64 | 2,89 | 14,56 |
| Houston | 0,23 | 0,81 | 3,14 | 15,16 |
| Generador de cuevas | 0,28 | 1,00 | 3,95 | 17,73 |
| Wilson | 0,34 | 1,69 | 5,26 | 26,84 |
| Aldous-Broder | 0,76 | 3,32 | 21,76 | 129,20 |

Cada columna tiene 4 veces más celdas que la anterior. Casi todos crecen de forma lineal: el tiempo se multiplica por 4 o poco más. Kruskal, Wilson, Houston y Aldous-Broder crecen algo más rápido, entre 5 y 6 veces de 200×200 a 400×400.

**`generate()` completo:**

| Algoritmo | 50×50 | 100×100 | 200×200 | 400×400 |
|-----------|------:|--------:|--------:|--------:|
| Binary Tree | 0,09 | 0,22 | 0,68 | 2,55 |
| Sidewinder | 0,10 | 0,24 | 0,77 | 2,90 |
| Recursive Division | 0,12 | 0,32 | 1,07 | 4,10 |
| DFS | 0,19 | 0,62 | 2,20 | 8,26 |
| Hunt-and-Kill | 0,16 | 0,53 | 1,92 | 9,17 |
| Eller | 0,21 | 0,71 | 2,74 | 11,22 |
| Growing Tree | 0,24 | 0,80 | 3,05 | 12,21 |
| Prim | 0,26 | 0,89 | 3,42 | 13,67 |
| Kruskal | 0,19 | 0,71 | 2,90 | 14,19 |
| Houston | 0,24 | 0,86 | 3,11 | 15,08 |
| Generador de cuevas | 0,38 | 1,65 | 5,25 | 25,61 |
| Wilson | 0,39 | 1,68 | 5,04 | 26,38 |
| Aldous-Broder | 0,80 | 3,55 | 21,79 | 123,74 |

## Cuánto cuesta cada fase

Medido por separado sobre un laberinto de Kruskal:

| Fase | 200×200 | 400×400 | Cuándo se ejecuta |
|------|--------:|--------:|-------------------|
| Algoritmo (Kruskal) | 2,65 | 13,38 | Siempre |
| Quitar callejones, dificultad 5 | 1,57 | 6,17 | Solo si la dificultad es menor que 10 |
| Habitaciones, nivel 0 | 0,11 | 0,37 | Salvo `rooms = false` |
| Habitaciones, nivel 10 | 0,68 | 2,79 | |
| Salidas (4 aleatorias) | 0,03 | 0,03 | Siempre |
| Validar las opciones | menos de 0,01 | menos de 0,01 | Siempre |
| Conexión, sin nada que unir | 1,70 | 6,63 | Solo en cuevas |

En una cueva de 400×400 con salidas, la conexión añade unos 4 ms a los 19 del algoritmo. El caso extremo, unir 20 000 islas de tres celdas en una rejilla de 400×400, tarda 14 ms.

---

## Qué se hizo

### O1. Compilar con optimización

Un `colcon build` normal no ponía tipo de compilación y el paquete se compilaba sin optimizar: DFS a 200×200 tardaba 103,7 ms en vez de 5,7 ms.

[CMakeLists.txt](../CMakeLists.txt) fija ahora `RelWithDebInfo` (`-O2 -g`) cuando no se indica otro. Las tablas de este documento no reflejan este cambio, porque ya se midieron con `-O2` antes y después.

### O2. Conexión de regiones en una sola pasada

El resolver original comprobaba la conectividad con un `std::unordered_set`, repetía ese recorrido una vez por salida y buscaba caminos con un conjunto que recorría entero en cada paso.

El actual ([region_connector.cpp](../src/region_connector.cpp)) usa vectores y resuelve todo en una pasada: etiqueta las regiones, las hace crecer todas a la vez y une los territorios por los puentes más baratos. Su coste no depende de cuántas regiones haya.

Además solo se ejecuta cuando puede hacer falta, que es con el generador de cuevas. Un laberinto perfecto es una única región y las demás fases solo abren muros, así que no hay nada que unir. Eso es lo que ha quitado casi todo el peso de las fases comunes.

### O3. Hunt-and-Kill no vuelve a mirar columnas completas

Cada caza recorría la rejilla desde el principio. Ahora recuerda la primera columna que todavía tiene celdas sin visitar y empieza desde ahí. De 1683 ms a 8,7 ms a 400×400, junto con O7 y O8.

### O4. Growing Tree elige sin barajar

Para elegir una celda activa al azar se barajaba la lista entera en cada paso. Ahora se elige un índice y se intercambia ese elemento con el último. De 1058 ms a 11,8 ms a 400×400.

### O5. Wilson: ni reinicios ni sorteos fallidos

Dos cambios:

- Al empezar cada paseo se rellenaba con -1 un vector con una entrada por celda. Sobraba: cada entrada ya se devuelve a -1 cuando su celda sale del camino.
- El punto de partida de cada paseo se buscaba probando celdas al azar hasta dar con una libre, y hacia el final casi todas están ocupadas. Ahora se toman en orden, lo que no cambia qué laberintos pueden salir.

De 436 ms a 26,8 ms a 400×400.

### O6. Generador de cuevas sin copias ni fuerza bruta

- **Dos vectores que se intercambian** en cada iteración, en vez de copiar la rejilla entera dos veces.
- **Una sola búsqueda en anchura desde el grupo principal** para encontrar la celda más cercana de cada grupo pequeño, en vez de comparar todas las celdas de un grupo con todas las del otro.
- **Recuento de vecinas con desplazamientos fijos**, sin construir una lista por celda.

De 480 ms a 17,7 ms a 400×400.

### O7. Vecinos sin reservar memoria

`neighbors` devuelve hasta 4 vecinos en un contenedor que vive en la pila. Antes devolvía un `std::vector` nuevo en cada llamada, y se llama una vez por paso en casi todos los algoritmos.

### O8. Un byte de muros por celda

Cada celda era un `std::variant` de tres tipos que ocupaba 3 bytes y comprobaba su tipo dos veces en cada acceso. Ahora la rejilla guarda un byte por celda y los algoritmos llevan sus propias marcas.

### O9. Mejoras menores

- **Prim** guarda las celdas visitadas en un vector en vez de un `std::unordered_set`.
- **Kruskal** reserva la lista de muros de una vez.
- **Eller** reutiliza un único vector para agrupar las celdas de cada fila.
- **Hunt-and-Kill y Growing Tree** eligen un vecino libre con un solo sorteo, en vez de barajar los cuatro.
- **El algoritmo por defecto** de `MazeGenerator` es DFS en vez de Aldous-Broder, que era 15 veces más lento.

### O10. Houston: Aldous-Broder y Wilson combinados

Aldous-Broder es lento por diseño y no tiene arreglo. El algoritmo nuevo, [Houston](algoritmos.md#houston), usa cada uno en la parte donde es rápido: 15 ms a 400×400, frente a 129 de Aldous-Broder y 27 de Wilson.

---

## El coste de los números aleatorios portables

Para que una semilla dé el mismo laberinto en cualquier compilador, la librería ya no usa `std::shuffle` ni las distribuciones estándar ([algoritmos.md](algoritmos.md#números-aleatorios-y-semillas)).

La primera versión propia hacía dos divisiones por sorteo y resultó claramente más lenta: Hunt-and-Kill pasó de 10,9 a 20,4 ms y Growing Tree de 16,1 a 24,9 ms a 400×400. Se sustituyó por el método de Lemire, que escala el número con una multiplicación y solo divide en casos raros.

Con eso, casi todo quedó igual o más rápido que con la biblioteca estándar. Tres algoritmos salen algo peor parados que en la ronda anterior de optimizaciones: Eller (un 6 % más lento), Kruskal (un 8 %) y Prim (un 17 %). Los tres dependen mucho de barajar, y el barajado de GCC ahorra sorteos con un truco que no es portable. Es un coste asumido a cambio de la reproducibilidad.

## Lo que queda

Nada urgente.

- **Aldous-Broder** sigue siendo el más lento con diferencia, por diseño. Para un laberinto sin sesgo conviene Wilson o Houston.
- **Quitar callejones** cuesta más o menos la mitad que generar el laberinto. Recorre la rejilla una vez para encontrarlos y consulta los vecinos de cada uno varias veces; se podría afinar si la dificultad se usa mucho.
- **Prim, Kruskal y Eller** podrían recuperar lo perdido con un barajado que saque dos índices de cada número aleatorio, a costa de complicar `Random`.
