#pragma once
#include "fuga_agents/wall_following_agent.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include <cmath>

namespace fuga_agents
{
class Pledge : public WallFollowingAgent {
public:
  Pledge();

private:
  geometry_msgs::msg::Twist
  decide(const sensor_msgs::msg::LaserScan::SharedPtr msg);

  geometry_msgs::msg::Twist go_straight();

  inline static float yaw_from(const geometry_msgs::msg::Quaternion & q)
  {
    return std::atan2(2.0 * (q.w * q.z + q.x * q.y),
                      1.0 - 2.0 * (q.y * q.y + q.z * q.z));
  }

  inline static float normalize(const float a)
  {
    return std::atan2(std::sin(a), std::cos(a));
  }

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr subscription_odom_;
  float turned_angle_ = 0.0f;
  float last_yaw_ = 0.0f;
  bool has_yaw_ = false;

  static constexpr float kEpsilon = 5.0f * std::numbers::pi_v<float>/ 180.0f;
  static constexpr float kK = 2.0f;

  enum class Mode { kStraight, kFollow };
  Mode mode_ = Mode::kStraight;
  bool armed_ = false; // ya se ha alejado de la pared al menos una vez
  static constexpr float kArm = 80.0f * std::numbers::pi_v<float>/ 180.0f;
};
} // namespace fuga_agents
