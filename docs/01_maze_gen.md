Colcon es el sistema de compilado de ros2, con el podemos crear los paquetes de ros2, a sea uniendo ficheros de pthon o con cmake_amend para los nodos/main de c++.
Un overlay es un workspace que modifica al sistema predeterminado de ros2, (jazzy), el cual se vuelve un underlay, ayq ue es la capa mas basica de ros2 que usamos, overlay es una capa que creamos para modifciar esa primera version de ros y darle mas funcionaldiad o incluso cambiar las bases.
Hay que hacer source porque el build de colcon crea paquetes, los cuales se tiene que "activar" para que esten disponibles.
