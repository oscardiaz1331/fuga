cmake_minimum_required(VERSION 3.8)
project(fuga_bringup)

find_package(ament_cmake REQUIRED)

# Paquete solo de launch: no compila nada, solo instala los launch files
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME})

ament_package()
