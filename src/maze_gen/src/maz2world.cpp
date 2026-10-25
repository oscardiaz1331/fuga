#include <exception>
#include "maze_gen/export.hpp"

// ros2 run maze_export maze2world --ros-args -p width:=10 -p height:=10 -p folder:=/tmp/maze
//   [-p seed:=42] [-p difficulty:=10] [-p start_x:=0] [-p start_y:=0]
// Escribe en la carpeta: maze.sdf, maze.pgm, maze.yaml y meta.yaml
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    // The node exports in its constructor: no spin, so the process ends when it is done
    // and the launch file can start Gazebo right after (OnProcessExit)
    auto node = std::make_shared<maze_gen::MazeExporterNode>();
  } catch (const std::exception & e) {
    // Exit code 1 so maze.launch.py stops instead of opening Gazebo with an old maze
    RCLCPP_FATAL(rclcpp::get_logger("maze_exporter"), "%s", e.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();

  return 0;
}
