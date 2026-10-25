cmake_minimum_required(VERSION 3.16)
project(fuga_eval LANGUAGES CXX)

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
find_package(geometry_msgs REQUIRED)
find_package(yaml-cpp REQUIRED)

# Logica del juez sin ROS (maquina de estados, meta.yaml, CSV), separada del
# nodo para poder probarla con gtest sin lanzar nada
add_library(fuga_eval_lib
  src/csv.cpp
  src/geometry.cpp
  src/judge.cpp
  src/map_capture.cpp
  src/meta.cpp
  src/path_length.cpp)
target_include_directories(fuga_eval_lib PUBLIC include)
target_link_libraries(fuga_eval_lib PUBLIC yaml-cpp::yaml-cpp)

# Nodo ROS del juez: envuelve fuga_eval_lib
add_executable(judge_node src/judge_node.cpp)
target_link_libraries(judge_node PRIVATE fuga_eval_lib rclcpp::rclcpp geometry_msgs::geometry_msgs)

install(TARGETS judge_node RUNTIME DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_map_capture test/test_map_capture.cpp)
  target_link_libraries(test_map_capture fuga_eval_lib)
endif()

ament_package()
