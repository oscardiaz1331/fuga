#include "maze_gen/export.hpp"

// ros2 run maze_export maze2world [ancho] [alto] [carpeta] [semilla]
// Escribe en la carpeta: maze.sdf, maze.pgm, maze.yaml y meta.yaml
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  // The node exports in its constructor: no spin, so the process ends when it is done
  // and the launch file can start Gazebo right after (OnProcessExit)
  auto node = std::make_shared<maze_gen::MazeExporterNode>();
  rclcpp::shutdown();

  return 0;
}
