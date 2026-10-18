cmake_minimum_required(VERSION 3.16)
project(maze_gen LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sin tipo de compilacion colcon no optimiza nada y la libreria va ~18x mas lenta
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Para cazar fallos de memoria: colcon build --cmake-args -DMAZE_GEN_SANITIZE=ON
option(MAZE_GEN_SANITIZE "Compila la libreria y los tests con AddressSanitizer y UBSan" OFF)
if(MAZE_GEN_SANITIZE)
  # -UNDEBUG deja activos tambien los assert de la propia libreria
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -D_GLIBCXX_ASSERTIONS -UNDEBUG)
  add_link_options(-fsanitize=address,undefined)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

# --- Libreria ---
# include/maze_gen/ es la interfaz publica; las cabeceras de src/ son internas y no se instalan
add_library(maze_gen
  src/algorithm_registry.cpp
  src/aldous_broder_strategy.cpp
  src/binary_tree_strategy.cpp
  src/box_drawing_chars.cpp
  src/cellular_automaton_strategy.cpp
  src/dead_end_braider.cpp
  src/dfs_maze_strategy.cpp
  src/ellers_strategy.cpp
  src/growing_tree_strategy.cpp
  src/houston_strategy.cpp
  src/hunt_and_kill_strategy.cpp
  src/kruskals_strategy.cpp
  src/maze_exit_generator.cpp
  src/maze_generator.cpp
  src/maze_grid.cpp
  src/prims_strategy.cpp
  src/random.cpp
  src/recursive_division_strategy.cpp
  src/region_connector.cpp
  src/room.cpp
  src/room_generator.cpp
  src/sidewinder_strategy.cpp
  src/union_find.cpp
  src/wilsons_strategy.cpp
)
add_library(maze_gen::maze_gen ALIAS maze_gen)
target_compile_features(maze_gen PUBLIC cxx_std_20)
# Necesario para poder enlazarla dentro de librerias compartidas (plugins, componentes)
set_target_properties(maze_gen PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(maze_gen
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# --- Demo: ros2 run maze_gen maze_gen_demo [carpeta_de_salida] ---
add_executable(maze_gen_demo src/mazes.cpp)
target_link_libraries(maze_gen_demo PRIVATE maze_gen)

# --- Instalacion y exportacion para otros paquetes ---
install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})

install(TARGETS maze_gen
  EXPORT maze_genTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS maze_gen_demo RUNTIME DESTINATION lib/${PROJECT_NAME})

ament_export_targets(maze_genTargets HAS_LIBRARY_TARGET)

# --- Tests: colcon test --packages-select maze_gen ---
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_uncrustify REQUIRED)

  set(MAZE_GEN_TESTS
    test_maze_grid
    test_maze_grid_load
    test_golden_mazes
    test_maze_algorithms
    test_maze_generator
    test_region_connector
    test_exits_and_rooms
    test_support_types
    test_reproducibility
  )
  foreach(test_name ${MAZE_GEN_TESTS})
    ament_add_gtest(${test_name} test/${test_name}.cpp TIMEOUT 120)
    target_link_libraries(${test_name} maze_gen)
    # Los tests tambien prueban las piezas internas
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  endforeach()

  # Laberintos de referencia guardados en disco: ver test/test_golden_mazes.cpp
  target_compile_definitions(test_golden_mazes PRIVATE
    MAZE_GEN_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/golden")

  # El formato del codigo (estilo de ROS 2) tambien se comprueba como un test
  ament_uncrustify()
endif()

ament_package()
