cmake_minimum_required(VERSION 3.8)
project(fuga_perception)

find_package(ament_cmake REQUIRED)
# Pin Ubuntu's OpenCV 4.6 (the one ROS Jazzy / cv_bridge use).
# Without the version, CMake picks the OpenCV 5 installed in /usr/local.
# Keep this before any package that depends on OpenCV (e.g. cv_bridge).
find_package(OpenCV 4.6 REQUIRED COMPONENTS core imgproc aruco)
find_package(rclcpp REQUIRED)
find_package(rclcpp REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(image_transport REQUIRED)

add_library(${PROJECT_NAME} src/aruco_opencv.cpp src/apriltag_finder.cpp)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>)
# Use the imported targets (pkg::pkg): they carry the include paths, the *_LIBS variables do not.
target_link_libraries(${PROJECT_NAME} PUBLIC
  ${OpenCV_LIBS}
  rclcpp::rclcpp
  cv_bridge::cv_bridge
  image_transport::image_transport
  tf2::tf2 
  tf2_ros::tf2_ros)

add_executable(apriltag_finder src/main.cpp)
target_link_libraries(apriltag_finder PRIVATE ${PROJECT_NAME})

install(DIRECTORY include/ DESTINATION include/${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME} EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS apriltag_finder RUNTIME DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY launch config DESTINATION share/${PROJECT_NAME})

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(OpenCV rclcpp cv_bridge image_transport)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
