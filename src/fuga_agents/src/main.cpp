#include "fuga_agents/wall_follower.hpp"
#include <rclcpp/rclcpp.hpp>
#include <memory>

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<fuga_agents::WallFollower>());
  rclcpp::shutdown();
}
