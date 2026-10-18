cmake_minimum_required(VERSION 3.16)
project(maze_export LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(maze_gen REQUIRED)

# El paquete se llama maze_export para no chocar con maze_gen (la libreria)
add_library(maze_export_lib src/export.cpp)
target_include_directories(maze_export_lib PUBLIC include)
target_link_libraries(maze_export_lib PUBLIC maze_gen::maze_gen rclcpp::rclcpp)

add_executable(maze2world src/maz2world.cpp)
target_link_libraries(maze2world PRIVATE maze_export_lib)

install(TARGETS maze2world RUNTIME DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_export test/test_export.cpp)
  target_link_libraries(test_export maze_export_lib)
endif()

ament_package()
