# Algoritmos de `maze_gen`

Cómo funciona cada algoritmo del paquete y cómo está implementado aquí.

- [Modelo de datos](#modelo-de-datos)
- [Qué hace `generate()`](#qué-hace-generate)
- [Resumen de los 13 algoritmos](#resumen-de-los-13-algoritmos)
- [Algoritmos de laberinto perfecto](#algoritmos-de-laberinto-perfecto)
- [Generador de cuevas](#generador-de-cuevas)
- [Fases comunes](#fases-comunes)
- [Números aleatorios y semillas](#números-aleatorios-y-semillas)

---

## Modelo de datos

**Rejilla.** `MazeGrid` es una rejilla de `ancho × alto` celdas, ambos al menos 1. La celda `(x, y)` tiene `x` hacia la derecha e `y` hacia **abajo**: el norte es `y - 1`.

**Muros.** Los muros están **entre** celdas, no ocupan celda. La rejilla guarda un byte por celda con un bit por lado; el bit a 1 significa "hay muro". Hay 4 direcciones (`Dir`): norte, este, sur y oeste. Dos celdas que solo se tocan por la esquina no son vecinas.

**Estado inicial.** Todas las celdas empiezan con todos los muros puestos.

**Operaciones** ([maze_grid.hpp](../include/maze_gen/maze_grid.hpp)):

| Operación | Efecto |
|-----------|--------|
| `carve(x, y, dir)` | Quita el muro entre la celda y su vecina. No hace nada si alguna está fuera de la rejilla |
| `add_wall(x, y, dir)` | Pone el muro entre la celda y su vecina |
| `add_exit(x, y, dir)` | Abre el muro exterior de una celda del borde y la apunta como salida. No hace nada si la dirección no sale de la rejilla o la salida ya existe |
| `walls(x, y)`, `has_wall(x, y, dir)` | Consulta |
| `exits()` | Las salidas, cada una un `Exit {x, y, dir}` |
| `neighbors(x, y)` | Las celdas vecinas que caen dentro de la rejilla, en orden norte, este, sur, oeste |
| `reset()` | Vuelve a poner todos los muros y borra las salidas |

El muro exterior solo se abre con `add_exit`. `carve` nunca lo toca, así que un error en un algoritmo no puede agujerear el borde.

**Datos de trabajo.** La rejilla solo guarda muros. Si un algoritmo necesita marcar celdas (visitadas, vivas), lleva su propio vector y lo descarta al terminar.

**Laberinto perfecto.** Es el que tiene exactamente un camino entre cualquier par de celdas: todo conectado y sin ciclos. En una rejilla de `N` celdas equivale a tener exactamente `N - 1` pasos abiertos. Es la propiedad que comprueban los tests para cada algoritmo.

---

## Qué hace `generate()`

[MazeGenerator::generate](../src/maze_generator.cpp) ejecuta estas fases, en este orden:

1. **Validación.** Comprueba todas las opciones. Si alguna no es válida lanza `std::invalid_argument` sin haber tocado la rejilla.
2. **Reinicio.** Pone todos los muros y borra las salidas anteriores.
3. **Algoritmo.** La estrategia elegida construye el laberinto.
4. **Callejones.** Si la dificultad es menor que 10, quita parte de los callejones sin salida.
5. **Habitaciones.** Abre zonas rectangulares sin muros interiores.
6. **Salidas.** Abre las salidas fijas y luego las aleatorias.
7. **Conexión.** Une las zonas que hayan quedado sueltas.

Las fases 4 y 5 solo se aplican a los 12 algoritmos de laberinto, y la 7 solo al generador de cuevas. Un laberinto perfecto es una única región, y las fases 4, 5 y 6 solo abren muros, así que no hay nada que unir. En una cueva sí puede hacer falta: una salida puede caer en roca maciza.

Con las opciones por defecto (dificultad 10, habitaciones activadas en su nivel más bajo), el resultado es un laberinto perfecto con alguna habitación: en 30×30 salen de media 4 pasos más que los 899 de un laberinto perfecto.

---

## Resumen de los 13 algoritmos

Los 12 algoritmos de laberinto generan laberintos perfectos: 20 de 20 en cada uno. Medido sobre el algoritmo solo. "Callejones" es el porcentaje de celdas con una única abertura, en 30×30 y de media sobre 20 semillas. El tiempo es para 200×200 (40 000 celdas) compilado con `-O2`.

| Algoritmo | Nombre | Callejones | Tiempo | Aspecto del laberinto |
|-----------|--------|-----------:|-------:|-----------------------|
| [DFS](#dfs-recursive-backtracker) | `dfs` | 10,2 % | 2,1 ms | Pasillos largos y retorcidos, pocas bifurcaciones |
| [Hunt-and-Kill](#hunt-and-kill) | `hunt_and_kill` | 9,8 % | 1,8 ms | Como DFS |
| [Aldous-Broder](#aldous-broder) | `aldous_broder` | 29,0 % | 21,8 ms | Sin sesgo |
| [Wilson](#wilson) | `wilson` | 29,5 % | 5,3 ms | Sin sesgo |
| [Houston](#houston) | `houston` | 29,5 % | 3,1 ms | Sin sesgo apreciable |
| [Kruskal](#kruskal) | `kruskal` | 30,8 % | 2,9 ms | Muchos callejones cortos, uniforme |
| [Prim](#prim) | `prim` | 32,5 % | 3,2 ms | Ramas cortas que salen de un centro |
| [Growing Tree](#growing-tree) | `growing_tree` | 27,5 % ¹ | 2,9 ms | Configurable: de Prim a DFS |
| [Binary Tree](#binary-tree) | `binary_tree` | 25,1 % | 0,5 ms | Sesgo diagonal muy marcado |
| [Sidewinder](#sidewinder) | `sidewinder` | 27,9 % | 0,6 ms | Pasillo recto arriba, sesgo vertical |
| [Eller](#eller) | `ellers` | 27,9 % | 2,6 ms | Se construye por filas; sin sesgo marcado |
| [Recursive Division](#recursive-division) | `recursive_division` | 27,2 % | 0,9 ms | Muros largos y rectos |
| [Generador de cuevas](#generador-de-cuevas) | `cellular_automaton` | n/a | 4,0 ms | Cueva, no laberinto |

¹ Con su opción por defecto.

El nombre es el que aceptan `algorithm_from_string` y `to_string`.

**Cuál usar.**

- Para un laberinto difícil de recorrer, con pasillos largos: **DFS** o **Hunt-and-Kill**.
- Para un laberinto sin ningún sesgo: **Wilson**, o **Houston** si importa la velocidad.
- Para muchos callejones cortos: **Kruskal** o **Prim**.
- Para generar muy rápido: **Binary Tree** o **Sidewinder**.
- Para un aspecto de plano de edificio: **Recursive Division**.
- Para zonas abiertas tipo cueva: el **generador de cuevas**.

El algoritmo por defecto de `MazeGenerator` es DFS.

---

## Algoritmos de laberinto perfecto

Cada uno es una clase declarada en [strategies.hpp](../src/strategies.hpp), con su implementación en un fichero propio.

### DFS (Recursive Backtracker)

**Idea.** Avanzar al azar hasta quedarse sin sitio y entonces retroceder.

1. Elegir una celda inicial al azar, marcarla visitada y meterla en una pila.
2. Mirar la celda de la cima. Si tiene vecinos sin visitar, elegir uno al azar, abrir el muro, marcarlo y apilarlo.
3. Si no tiene, desapilar.
4. Repetir hasta vaciar la pila.

**Implementación.** [dfs_maze_strategy.cpp](../src/dfs_maze_strategy.cpp). Usa una pila explícita, así que no hay riesgo de desbordar la pila de llamadas con laberintos grandes.

**Coste.** Lineal en el número de celdas.

**Aspecto.** Pasillos muy largos y pocos callejones (10,2 %, un tercio que la mayoría).

### Hunt-and-Kill

**Idea.** Como DFS pero sin pila: cuando se queda sin salida, en vez de retroceder busca por la rejilla un punto desde el que continuar.

1. Pasear al azar abriendo muros hacia celdas sin visitar ("kill").
2. Al quedarse sin vecinos libres, recorrer la rejilla hasta encontrar una celda sin visitar que tenga un vecino visitado ("hunt").
3. Abrir el muro hacia uno de sus vecinos visitados, elegido al azar, y seguir paseando desde ella.
4. Terminar cuando todas las celdas estén visitadas.

**Implementación.** [hunt_and_kill_strategy.cpp](../src/hunt_and_kill_strategy.cpp). La caza recorre la rejilla por columnas y recuerda la primera columna que aún tiene celdas sin visitar, para no volver a mirar las que ya están completas.

**Coste.** Lineal en la práctica.

**Aspecto.** Muy parecido a DFS: pasillos largos y 9,8 % de callejones.

### Aldous-Broder

**Idea.** Un paseo completamente aleatorio; solo se abre muro al entrar en una celda nueva.

1. Elegir una celda inicial al azar y marcarla.
2. Moverse a un vecino al azar (visitado o no).
3. Si el vecino no estaba visitado, abrir el muro por el que se ha entrado y marcarlo.
4. Repetir hasta haber visitado todas las celdas.

**Implementación.** [aldous_broder_strategy.cpp](../src/aldous_broder_strategy.cpp).

**Coste.** No está acotado: el paseo puede tardar mucho en encontrar las últimas celdas. A 400×400 tarda 129 ms, 15 veces más que DFS. Es lento por diseño.

**Aspecto.** Genera todos los laberintos perfectos posibles con la misma probabilidad. No tiene ningún sesgo.

### Wilson

**Idea.** Igual de imparcial que Aldous-Broder, pero más rápido. Añade al laberinto paseos aleatorios a los que se les borran los bucles.

1. Marcar una celda al azar como parte del laberinto.
2. Elegir una celda que aún no esté en el laberinto.
3. Pasear al azar desde ella hasta tocar el laberinto. Si el paseo pasa por una celda por la que ya pasó, borrar el bucle que se ha formado.
4. Abrir los muros a lo largo del camino resultante y añadir sus celdas al laberinto.
5. Repetir desde 2 hasta que no queden celdas.

**Implementación.** [wilsons_strategy.cpp](../src/wilsons_strategy.cpp).

- Guarda la posición de cada celda dentro del camino actual, para detectar un bucle sin recorrer el camino.
- Al borrar un bucle descarta también el paso de la celda donde se cerró, que se vuelve a añadir con su dirección nueva.
- En el paso 2 toma las celdas libres en orden en vez de probar al azar. El punto de partida de cada paseo no cambia qué laberintos pueden salir, y probar al azar se vuelve lento cuando casi todo está ocupado.

**Coste.** Lento al principio, cuando el laberinto es un punto difícil de encontrar, y rápido al final. A 400×400 tarda 27 ms.

**Aspecto.** Sin sesgo. Su porcentaje de callejones coincide con el de Aldous-Broder, como debe ser: los dos producen la misma distribución de laberintos.

### Houston

**Idea.** Aldous-Broder es rápido al principio, cuando casi todas las celdas son nuevas, y se arrastra al final. Wilson es justo al revés. Houston empieza con uno y termina con el otro.

1. Aplicar Aldous-Broder hasta que un tercio de las celdas esté en el laberinto.
2. Completar el resto con Wilson.

**Implementación.** [houston_strategy.cpp](../src/houston_strategy.cpp). La segunda parte es la misma función que usa Wilson.

**Coste.** A 400×400 tarda 15 ms: 8 veces menos que Aldous-Broder y casi la mitad que Wilson.

**Aspecto.** Igual que los otros dos a efectos prácticos: su 29,5 % de callejones coincide con el de Wilson. No hay demostración de que la distribución sea exactamente uniforme.

### Kruskal

**Idea.** Tratar cada muro interior como una posible unión y aceptar, en orden aleatorio, las que unan dos zonas todavía separadas.

1. Hacer una lista con todos los muros interiores y barajarla.
2. Cada celda empieza en su propio conjunto.
3. Para cada muro: si las dos celdas que separa están en conjuntos distintos, abrirlo y unir los conjuntos.

**Implementación.** [kruskals_strategy.cpp](../src/kruskals_strategy.cpp), con la estructura de conjuntos disjuntos de [union_find.cpp](../src/union_find.cpp) (compresión de caminos y unión por rango).

**Coste.** Prácticamente lineal. Necesita memoria para la lista completa de muros (unos `2N`).

**Aspecto.** Crece desde muchos puntos a la vez. Muchos callejones cortos (30,8 %), textura uniforme.

### Prim

**Idea.** Hacer crecer el laberinto desde una celda, abriendo cada vez un muro al azar de su frontera.

1. Marcar una celda al azar y añadir a la lista de frontera los muros que la separan de celdas de fuera.
2. Sacar un muro al azar de la lista.
3. Si la celda del otro lado sigue fuera del laberinto, abrirlo, marcarla y añadir sus muros a la lista.
4. Repetir hasta vaciar la lista.

**Implementación.** [prims_strategy.cpp](../src/prims_strategy.cpp). Saca el muro elegido intercambiándolo con el último, sin desplazar la lista.

**Coste.** Lineal.

**Aspecto.** Ramas cortas que irradian desde el punto de inicio. Es el que más callejones produce (32,5 %) y el más fácil de resolver.

### Growing Tree

**Idea.** Un algoritmo general con una lista de celdas activas. Según cómo se elija la siguiente celda de la lista, se comporta como otros algoritmos.

1. Meter una celda al azar en la lista de activas.
2. Elegir una celda de la lista.
3. Si tiene vecinos sin visitar, abrir el muro hacia uno al azar y añadirlo a la lista.
4. Si no, quitarla de la lista.
5. Repetir hasta vaciar la lista.

La gracia está en el paso 2: elegir siempre la **más reciente** da DFS; elegir **al azar** da algo parecido a Prim.

**Implementación.** [growing_tree_strategy.cpp](../src/growing_tree_strategy.cpp). La opción `growing_tree_newest_ratio` (de 0 a 1) es la probabilidad de tomar la más reciente en vez de una al azar. Vale 0 por defecto.

| `growing_tree_newest_ratio` | 0 | 0,25 | 0,5 | 0,75 | 0,9 | 1 |
|-----------------------------|--:|-----:|----:|-----:|----:|--:|
| Callejones | 27,5 % | 31,0 % | 32,4 % | 27,0 % | 18,8 % | 10,6 % |

El porcentaje de callejones no baja de forma continua: alcanza su máximo hacia la mitad y solo cae con claridad a partir de 0,75. Con 1 da un laberinto como el de DFS.

**Coste.** Lineal.

### Binary Tree

**Idea.** El más simple: cada celda abre un único muro, hacia el norte o hacia el oeste.

1. Para cada celda, elegir al azar entre norte y oeste y abrir ese muro.
2. En la fila superior solo se puede ir al oeste; en la columna izquierda solo al norte; la esquina no abre nada.

**Implementación.** [binary_tree_strategy.cpp](../src/binary_tree_strategy.cpp).

**Coste.** Lineal y sin memoria extra: cada celda se decide sin mirar a las demás. Es el más rápido (2 ms a 400×400).

**Aspecto.** Sesgo diagonal muy visible. La fila superior y la columna izquierda son dos pasillos rectos, y desde cualquier celda se llega a la esquina yendo solo al norte y al oeste.

### Sidewinder

**Idea.** Parecido a Binary Tree pero trabajando por tramos horizontales, lo que elimina uno de los dos pasillos rectos.

1. La fila superior es un único pasillo.
2. En cada fila siguiente, ir de izquierda a derecha acumulando celdas en un "tramo".
3. En cada celda, con cierta probabilidad abrir hacia el este y seguir el tramo.
4. Si no, cerrar el tramo: elegir una de sus celdas al azar, abrir hacia el norte desde ella y empezar un tramo nuevo.

**Implementación.** [sidewinder_strategy.cpp](../src/sidewinder_strategy.cpp). La probabilidad del paso 3 es la opción `sidewinder_east_probability`, 0,5 por defecto. Con 0 cada celda abre hacia el norte; con 1 cada fila es un único tramo con una sola subida.

**Coste.** Lineal; solo necesita recordar el tramo actual.

**Aspecto.** Un pasillo recto en la fila de arriba y sesgo vertical: desde cualquier celda se llega arriba sin retroceder nunca hacia el sur.

### Eller

**Idea.** Construir el laberinto fila a fila recordando solo qué celdas de la fila actual están ya conectadas entre sí.

1. En la fila actual, unir al azar celdas contiguas que pertenezcan a conjuntos distintos.
2. Para cada conjunto de la fila, abrir hacia abajo al menos una de sus celdas.
3. Pasar a la fila siguiente. Las celdas a las que no se ha bajado empiezan en un conjunto nuevo.
4. En la última fila, unir todas las celdas contiguas que sigan en conjuntos distintos.

**Implementación.** [ellers_strategy.cpp](../src/ellers_strategy.cpp). La probabilidad del paso 1 es la opción `ellers_join_probability`, 0,5 por defecto. El número de bajadas por conjunto se elige uniforme entre 1 y el tamaño del conjunto. Los conjuntos de cada fila se recorren en un orden fijo, obtenido al ordenar las celdas por conjunto.

El atractivo teórico del algoritmo es que solo necesita memoria para una fila. Esta implementación usa una estructura de conjuntos para la rejilla completa, así que el resultado es correcto pero no aprovecha esa ventaja.

**Coste.** Lineal.

### Recursive Division

**Idea.** Al revés que los demás: empezar con un espacio vacío e ir levantando muros.

1. Partir de una zona sin muros interiores.
2. Dividirla con un muro recto que tenga un único paso.
3. Repetir en cada una de las dos mitades hasta que no se puedan dividir más.

**Implementación.** [recursive_division_strategy.cpp](../src/recursive_division_strategy.cpp).

- Como la rejilla empieza con todos los muros, el primer paso es abrir todos los interiores.
- Cada zona se corta por su lado más largo; si es cuadrada, se elige al azar.
- La posición del muro y la del paso se eligen al azar.
- Una zona de una celda de ancho ya es un pasillo y no se divide más.

**Coste.** Lineal.

**Aspecto.** Muros largos y rectos, aspecto de plano de oficina. Es fácil de resolver a la vista porque cada muro tiene un único paso.

---

## Generador de cuevas

No genera un laberinto perfecto sino una **cueva**: zonas abiertas irregulares separadas por bloques macizos. Es un autómata celular, implementado en [cellular_automaton_strategy.cpp](../src/cellular_automaton_strategy.cpp).

**Fases.**

1. **Inicio.** Cada celda queda "viva" (abierta) o "muerta" (roca) al azar.
2. **Reglas.** Se aplica 5 veces una regla al estilo del Juego de la Vida, contando las 4 vecinas: una celda muerta nace si tiene exactamente 3 vivas, y una viva muere si no tiene ninguna. Las celdas de fuera de la rejilla cuentan como muertas.
3. **Conectividad.** Se buscan los grupos de celdas vivas y cada uno se une al más grande con un pasillo en forma de L hasta su celda más cercana.
4. **Cierre.** Se abre el paso entre cada par de celdas vivas vecinas. Las muertas conservan todos sus muros.

Durante las tres primeras fases el estado vivo o muerto se guarda en un vector propio; la rejilla solo se toca en la fase 4.

**Cuánta roca.** La opción `cave_level` (de 0 a 10, por defecto 5) fija la probabilidad de que una celda empiece muerta, del 35 % al 70 %. La regla es la misma en todos los niveles, así que la roca del resultado crece de forma continua:

| `cave_level` | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|-------------|--:|--:|--:|--:|--:|--:|--:|--:|--:|--:|---:|
| Celdas de roca | 16 % | 19 % | 22 % | 27 % | 32 % | 37 % | 42 % | 45 % | 48 % | 50 % | 52 % |

El resultado tiene menos roca que el estado inicial porque la fase 3 abre pasillos entre los grupos, y hay más grupos sueltos cuanta más roca hay.

**Observaciones.**

- La cueva es siempre una sola región: en todas las pruebas, a todos los niveles.
- Dos celdas abiertas solo se comunican si comparten un lado. No hay pasos en diagonal.
- `difficulty`, `rooms` y `room_level` no le afectan.

---

## Fases comunes

### Callejones sin salida

[dead_end_braider.cpp](../src/dead_end_braider.cpp). Es lo que controla la opción `difficulty`, de 0 a 10.

1. Se hace una lista con todos los callejones (celdas con una sola abertura) y se baraja.
2. Cada uno se elimina con probabilidad `(10 - difficulty) / 10`.
3. Eliminarlo es abrirle un muro más. Si alguno de sus vecinos tapiados es también un callejón, se abre hacia él: así desaparecen dos con un solo muro.

Con dificultad 10 no se hace nada y el laberinto queda perfecto. Con 0 se eliminan todos. Medido con Kruskal en 30×30, sin habitaciones:

| `difficulty` | 10 | 8 | 6 | 4 | 2 | 0 |
|-------------|---:|--:|--:|--:|--:|--:|
| Callejones | 30,8 % | 20,8 % | 13,2 % | 7,1 % | 2,9 % | 0 % |
| Pasos abiertos | 899 | 950 | 991 | 1028 | 1057 | 1080 |

Solo se abren muros: el laberinto sigue siendo una sola región y el borde no se toca. Los extremos de un pasillo de una celda de ancho (un laberinto de 1×N) no se pueden eliminar, porque no tienen otro muro que abrir.

### Habitaciones

[room_generator.cpp](../src/room_generator.cpp). Se desactivan con `rooms = false`.

1. **Configuración.** A partir de `room_level` (de 0 a 10) y del tamaño del laberinto se calculan el tamaño mínimo y máximo de habitación, cuántas se intentan y qué fracción del laberinto pueden ocupar (del 5 % al 25 %).
2. **Colocación.** Se prueban posiciones al azar. Una habitación se acepta si no toca a otra (con una celda de margen) y no supera el área máxima. Hay un límite de 50 intentos por habitación.
3. **Vaciado.** Se quitan todos los muros interiores de cada habitación.
4. **Puertas.** Se abren de 1 a 3 pasos desde celdas del perímetro hacia fuera, según el tamaño de la habitación.

Las habitaciones dejan siempre margen con el borde: una celda al norte y al oeste, dos al sur y al este.

| `room_level` | 0 | 5 | 10 |
|-------------|--:|--:|---:|
| Pasos de más sobre un laberinto perfecto de 30×30 | 4 | 33 | 179 |

Si el laberinto es demasiado pequeño para que quepa una habitación (menos de 5 o 6 celdas en alguna dimensión, según el nivel), esta fase no hace nada.

### Salidas

[maze_exit_generator.cpp](../src/maze_exit_generator.cpp). Hay dos tipos, que se pueden combinar:

- **Fijas** (`fixed_exits`): se dan como `Exit {x, y, dirección}`. Se abren primero y en su orden. Pueden estar en una esquina.
- **Aleatorias** (`num_random_exits`): se eligen al azar entre los huecos libres. Un hueco es cada lado exterior de una celda del borde que no sea esquina.

Nunca se repite una salida: las aleatorias evitan los huecos que ya ocupa una fija.

La validación rechaza, antes de tocar la rejilla:

- Una salida fija fuera de la rejilla, que no apunte hacia fuera, repetida o con una dirección que no existe.
- Un número de salidas aleatorias negativo, o mayor que los huecos libres. Una rejilla de 6×4 tiene 12 huecos; una de 2×2, ninguno.

### Conexión

[region_connector.cpp](../src/region_connector.cpp). Garantiza que no queda ninguna parte incomunicada. Solo se ejecuta con el generador de cuevas.

1. **Etiquetar regiones.** Un recorrido de la rejilla asigna a cada celda el número de su región: las celdas unidas por pasos abiertos comparten número.
2. **Decidir cuáles hay que unir.** Las regiones de dos o más celdas y las que contienen una salida. Una celda suelta sin ninguna abertura es roca y se deja como está.
3. **Repartir el terreno.** Todas esas regiones crecen a la vez, celda a celda, a través de la roca. Cada celda queda asignada a la región que llega a ella rompiendo menos muros.
4. **Buscar puentes.** Donde se tocan los territorios de dos regiones hay un posible puente, cuyo coste son los muros que hay que romper para unirlas por ahí.
5. **Elegir los más baratos.** Se toman los puentes de menor a mayor coste, saltando los que unen regiones ya unidas, hasta que todo es una sola región.

Todo se resuelve en una sola pasada, haya dos regiones o veinte mil. Devuelve el número de muros rotos.

---

## Números aleatorios y semillas

[random.hpp](../src/random.hpp). Con la misma semilla y las mismas opciones sale siempre el mismo laberinto, en cualquier compilador. Sin semilla, cada laberinto es distinto.

**Por qué una clase propia.** El generador `std::mt19937` está especificado bit a bit, pero `std::shuffle` y las distribuciones de la biblioteca estándar no: GCC, Clang y MSVC dan resultados distintos con el mismo generador. La clase `Random` usa `std::mt19937` y hace por su cuenta el resto: enteros en un rango, probabilidades y barajado.

**Una secuencia por fase.** El algoritmo, los callejones, las habitaciones y las salidas sacan sus números de secuencias distintas, todas derivadas de la misma semilla. Consecuencia práctica: cambiar una fase no altera las demás. Por ejemplo, desactivar las habitaciones deja las salidas donde estaban.

**Cuándo cambia el laberinto de una semilla.** Cuando se modifica un algoritmo de forma que consuma los números de otra manera. Los tests de [test_reproducibility.cpp](../test/test_reproducibility.cpp) fijan el laberinto exacto de cada algoritmo para una semilla, así que un cambio así no pasa desapercibido.
