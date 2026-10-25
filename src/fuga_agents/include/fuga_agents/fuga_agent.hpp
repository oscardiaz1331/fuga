#pragma once

#include "rclcpp/node.hpp"

namespace fuga_agents
{
class FugaAgent : public rclcpp::Node {
protected:
  FugaAgent(const std::string & name)
  : rclcpp::Node(name) {}
};

} // namespace fuga_agents
