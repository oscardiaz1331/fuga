#pragma once

#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"

#include "fuga_agents/wall_following_agent.hpp"

namespace fuga_agents {
class WallFollower : public WallFollowingAgent {
public:
  WallFollower();
  private:
  geometry_msgs::msg::Twist
  decide(const sensor_msgs::msg::LaserScan::SharedPtr msg);
};
} // namespace fuga_agents
