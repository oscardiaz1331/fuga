#pragma once

#include "fuga_agents/fuga_agent.hpp"
#include "fuga_cpp_navigate_to_pose/navigate_to_pose.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

namespace fuga_agents
{
class FrontierExplorer : public FugaAgent
{
public:
  FrontierExplorer();

private:
  void map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);

  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr subscription_;
  std::shared_ptr<fuga_cpp_navigate_to_pose::NavigateToPoseCpp> navigator_;

  fuga_cpp_navigate_to_pose::NavigateToPoseCpp::ResultFuture current_goal_result_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::unique_ptr<tf2_ros::TransformListener> tf_listener_;

  struct Point
  {
    double x;
    double y;
  };
  static constexpr double kFailedRadius = 0.5;  // m
  std::optional<Point> current_target_;  // en metros
  std::vector<Point> failed_targets_;    // en metros: sobrevive a cambios de origen del mapa
};
}  // namespace fuga_agents