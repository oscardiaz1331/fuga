#pragma once

#include "fuga_agents/wall_following_agent.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"

namespace fuga_agents
{
class WallFollower : public WallFollowingAgent
{
public:
  WallFollower();

private:
  void do_step() override;
  geometry_msgs::msg::Twist decide(const sensor_msgs::msg::LaserScan::SharedPtr msg);
  sensor_msgs::msg::LaserScan::SharedPtr laser_msg_;
};
}  // namespace fuga_agents
