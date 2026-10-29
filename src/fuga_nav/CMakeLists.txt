cmake_minimum_required(VERSION 3.8)
project(fuga_nav)

find_package(ament_cmake REQUIRED)

# Sin esto launch y config no llegan a install/ y FindPackageShare no los encuentra
install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
