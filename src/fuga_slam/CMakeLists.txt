cmake_minimum_required(VERSION 3.8)
project(fuga_slam)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)

# Sin esto launch y config no llegan a install/ y FindPackageShare no los encuentra
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})
# En lib/<paquete> para que lo encuentre ros2 run fuga_slam map_iou.py
install(PROGRAMS scripts/map_iou.py DESTINATION lib/${PROJECT_NAME})

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

  find_package(ament_cmake_pytest REQUIRED)
  ament_add_pytest_test(test_map_iou test/test_map_iou.py)
endif()

ament_package()
