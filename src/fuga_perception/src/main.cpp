#include <rclcpp/rclcpp.hpp>
#include "fuga_perception/apriltag_finder.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<fuga_perception::AprilTagFinder>());
  rclcpp::shutdown();
}
