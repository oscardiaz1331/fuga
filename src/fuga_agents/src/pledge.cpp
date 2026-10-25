#include "fuga_agents/pledge.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include <algorithm>

fuga_agents::Pledge::Pledge()
: WallFollowingAgent("pledge")
{
  subscription_odom_ = this->create_subscription<nav_msgs::msg::Odometry>(
      "odom", 10, [this](const nav_msgs::msg::Odometry::SharedPtr odometry) {
      const float yaw = yaw_from(odometry->pose.pose.orientation);
      if (has_yaw_) {
        turned_angle_ += normalize(yaw - last_yaw_);
      } else {
        has_yaw_ = true;
      }
      last_yaw_ = yaw;
      });
  auto callback = [this](const sensor_msgs::msg::LaserScan::SharedPtr msg) {
      publisher_->publish(decide(msg));
    };
  subscription_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
      "scan", 10, callback);
}

geometry_msgs::msg::Twist fuga_agents::Pledge::go_straight()
{
  geometry_msgs::msg::Twist twist;
  twist.linear.x = params_.velocity;
  twist.angular.z = std::clamp(-kK * turned_angle_, -params_.max_angular,
                               params_.max_angular);
  return twist;
}

geometry_msgs::msg::Twist
fuga_agents::Pledge::decide(const sensor_msgs::msg::LaserScan::SharedPtr msg)
{
  const float away = -derived_.side_sign * turned_angle_;
  switch (mode_) {
    case Mode::kStraight:
      if (!front_wall_detected(*msg)) {
        return go_straight();
      }
      mode_ = Mode::kFollow;
      armed_ = false;
      [[fallthrough]];
    case Mode::kFollow:
      if (!armed_) {
        if (away < kArm) {
          geometry_msgs::msg::Twist twist;
          twist.angular.z = -derived_.side_sign * params_.turn_speed;
          return twist;
        }
        armed_ = true;
      }
      if (away <= kEpsilon) {
        mode_ = Mode::kStraight;
        return go_straight();
      }
      return follow_wall(msg);
  }
  RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 1000,
                        "Unknown mode %d, stopping", static_cast<int>(mode_));
  return geometry_msgs::msg::Twist{};
}
