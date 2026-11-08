#include "fuga_agents/wall_follower.hpp"

#include <rclcpp/create_subscription.hpp>

fuga_agents::WallFollower::WallFollower()
: WallFollowingAgent("wall_follower")
{
  auto callback = [this](const sensor_msgs::msg::LaserScan::SharedPtr msg) {
    laser_msg_ = msg;
    step();
  };
  subscription_ = this->create_subscription<sensor_msgs::msg::LaserScan>("scan", 10, callback);
}

void fuga_agents::WallFollower::do_step()
{
  publisher_->publish(follow_wall(laser_msg_));
}