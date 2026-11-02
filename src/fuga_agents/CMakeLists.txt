cmake_minimum_required(VERSION 3.16)
project(fuga_agents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(fuga_cpp_navigate_to_pose REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)

# Deteccion de fronteras: sin dependencias de ROS, en su propia libreria para poder testearla
add_library(fuga_frontiers STATIC src/frontiers.cpp)
target_include_directories(fuga_frontiers PUBLIC include)

add_executable(fuga_agents src/main.cpp src/wall_following_agent.cpp src/wall_follower.cpp src/pledge.cpp src/frontier_explorer.cpp)
target_include_directories(fuga_agents PUBLIC include)
target_link_libraries(fuga_agents PUBLIC rclcpp::rclcpp sensor_msgs::sensor_msgs geometry_msgs::geometry_msgs nav_msgs::nav_msgs fuga_cpp_navigate_to_pose::navigate_to_pose fuga_frontiers tf2::tf2 tf2_ros::tf2_ros)

install(TARGETS fuga_agents RUNTIME DESTINATION lib/${PROJECT_NAME})


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
  ament_add_gtest(test_frontiers test/test_frontiers.cpp)
  target_link_libraries(test_frontiers fuga_frontiers)
endif()

ament_package()
