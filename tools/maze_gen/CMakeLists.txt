cmake_minimum_required(VERSION 3.16)
project(maze_gen LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)

# --- Libreria: todo src/ menos mazes.cpp, que es el main() de la demo ---
file(GLOB MAZE_GEN_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM MAZE_GEN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/mazes.cpp")

add_library(maze_gen ${MAZE_GEN_SOURCES})
add_library(maze_gen::maze_gen ALIAS maze_gen)
target_compile_features(maze_gen PUBLIC cxx_std_20)
# Necesario para poder enlazarla dentro de librerias compartidas (plugins, componentes)
set_target_properties(maze_gen PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(maze_gen PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

# --- Demo: genera un laberinto por algoritmo en ./data/<Algoritmo>.txt ---
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

ament_package()
