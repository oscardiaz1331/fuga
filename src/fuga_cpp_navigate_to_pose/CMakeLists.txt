cmake_minimum_required(VERSION 3.16)
project(fuga_cpp_navigate_to_pose LANGUAGES CXX)

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
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(rclcpp_action REQUIRED)

# Libreria: el cliente NavigateToPoseCpp, para usarlo desde otros paquetes
add_library(navigate_to_pose SHARED src/navigate_to_pose.cpp)
target_include_directories(navigate_to_pose PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
# PUBLIC: el header usa estos tipos, asi que quien enlace la libreria tambien los necesita
target_link_libraries(navigate_to_pose PUBLIC
  rclcpp::rclcpp rclcpp_action::rclcpp_action
  ${geometry_msgs_TARGETS} ${nav2_msgs_TARGETS})

# Ejecutable: solo el main, enlazado a la libreria
add_executable(fuga_cpp_navigate_to_pose src/main.cpp)
target_link_libraries(fuga_cpp_navigate_to_pose PRIVATE navigate_to_pose)

install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})
install(TARGETS navigate_to_pose
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS fuga_cpp_navigate_to_pose DESTINATION lib/${PROJECT_NAME})

# Para que otros paquetes hagan find_package(fuga_cpp_navigate_to_pose) y enlacen
# fuga_cpp_navigate_to_pose::navigate_to_pose
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp rclcpp_action geometry_msgs nav2_msgs)

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
endif()

ament_package()
