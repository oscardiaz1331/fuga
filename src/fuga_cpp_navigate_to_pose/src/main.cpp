#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "fuga_cpp_navigate_to_pose/navigate_to_pose.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  const auto node = std::make_shared<rclcpp::Node>("navigate_to_pose_cpp");
  fuga_cpp_navigate_to_pose::NavigateToPoseCpp navigator(*node);

  geometry_msgs::msg::PoseStamped pose{};
  pose.header.frame_id = "map";
  pose.header.stamp = node->now();
  pose.pose.position.x = 0.0;
  pose.pose.position.y = 0.0;

  const auto result = navigator.send_goal(pose);
  // Hace spin hasta que termine la navegación (o hasta Ctrl+C).
  const auto spin_rc = rclcpp::spin_until_future_complete(node, result);

  const bool succeeded = spin_rc == rclcpp::FutureReturnCode::SUCCESS &&
    result.get() == rclcpp_action::ResultCode::SUCCEEDED;
  rclcpp::shutdown();
  return succeeded ? 0 : 1;
}
