#include <memory>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <vector>

#include "fuga_agents/frontier_explorer.hpp"
#include "fuga_agents/fuga_agent.hpp"
#include "fuga_agents/pledge.hpp"
#include "fuga_agents/wall_follower.hpp"

// Uso: ros2 run fuga_agents fuga_agents [wall_follower|pledge|frontier_explorer] [--ros-args ...]
// frontier_explorer necesita SLAM (/map) y Nav2 (navigate_to_pose)
// Es un argumento y no un parametro ROS porque hay que saberlo antes de crear
// el nodo, y los parametros son de un nodo concreto
int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  // Quita --ros-args ... y deja el resto; args[0] es el nombre del programa
  const std::vector<std::string> args = rclcpp::remove_ros_arguments(argc, argv);
  const std::string algorithm = args.size() > 1 ? args[1] : "wall_follower";

  std::shared_ptr<fuga_agents::FugaAgent> agent;
  if (algorithm == "wall_follower") {
    agent = std::make_shared<fuga_agents::WallFollower>();
  } else if (algorithm == "pledge") {
    agent = std::make_shared<fuga_agents::Pledge>();
  } else if (algorithm == "frontier_explorer") {
    agent = std::make_shared<fuga_agents::FrontierExplorer>();
  } else {
    // Mejor fallar que lanzar otro: el juez apuntaria en el CSV el que no es
    RCLCPP_FATAL(
      rclcpp::get_logger("fuga_agents"),
      "Algoritmo desconocido '%s'. Opciones: wall_follower, pledge, frontier_explorer",
      algorithm.c_str());
    rclcpp::shutdown();
    return 1;
  }

  RCLCPP_INFO(agent->get_logger(), "Agente: %s", algorithm.c_str());
  rclcpp::spin(agent);
  rclcpp::shutdown();
  return 0;
}
