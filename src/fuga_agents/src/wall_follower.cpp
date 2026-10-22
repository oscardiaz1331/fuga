#include "fuga_agents/wall_follower.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <rclcpp/create_subscription.hpp>

fuga_agents::WallFollower::WallFollower() : rclcpp::Node("wall_follower") {
  publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
  auto callback = [this](const sensor_msgs::msg::LaserScan::SharedPtr msg) {
    this->publisher_->publish(this->decide(msg));
  };
  subscription_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
      "scan", 10, callback);
}

geometry_msgs::msg::Twist fuga_agents::WallFollower::decide(
    const sensor_msgs::msg::LaserScan::SharedPtr msg) {
  if (exited_) {
    geometry_msgs::msg::Twist twist;
    twist.linear.x = 0.0;
    twist.angular.z = 0.0;
    return twist;
  }
  const float angle_min = msg->angle_min;
  const float angle_max = msg->angle_max;
  const float angle_increment = msg->angle_increment;
  const auto begin = msg->ranges.begin();
  const auto angle_to_index = [angle_min, angle_max, angle_increment,
                               this](float angle) {
    check_angle_ranges(angle_min, angle_max, angle);
    return static_cast<int>((angle - angle_min) / angle_increment);
  };
  const float distance_right = median(
      std::vector<float>(begin + angle_to_index(angle_right_window_start_),
                         begin + angle_to_index(angle_right_window_end_)));
  const float distance_front_right = median(std::vector<float>(
      begin + angle_to_index(angle_front_right_window_start_),
      begin + angle_to_index(angle_front_right_window_end_)));

  const bool front_wall_detected = is_wall_detected(
      std::vector<float>(begin + angle_to_index(angle_front_window_start_),
                         begin + angle_to_index(angle_front_window_end_)),
      distance_front_threshold_);
  const bool right_wall_detected = distance_right < distance_lateral_threshold_;
  // Twist
  geometry_msgs::msg::Twist twist;

  if (front_wall_detected) {
    // 1. Esquina interior / callejón: girar a la izquierda en el sitio
    twist.linear.x = 0.0;
    twist.angular.z = turn_speed_;
  } else if (!right_wall_detected) {
    // 2. Esquina exterior: curva a la derecha con radio = distance_target_
    twist.linear.x = corner_velocity_;
    twist.angular.z = -twist.linear.x / distance_target_;
  } else {
    // 3. Seguir la pared con el PD
    float angle = 0.0f; // sin lectura fiable en diagonal, ir recto
    if (std::isfinite(distance_front_right) &&
        distance_front_right < distance_front_right_threshold_) {
      angle = std::atan2(distance_front_right * std::cos(angle_delta_) -
                             distance_right,
                         distance_front_right * std::sin(angle_delta_));
    }
    const float distance_to_wall = distance_right * std::cos(angle);
    const float correction =
        -kp_ * (distance_to_wall - distance_target_) - kd_ * angle;
    twist.linear.x = velocity_;
    twist.angular.z = std::clamp(correction, -max_angular_, max_angular_);
  }

  const bool left_wall_detected = is_wall_detected(
      std::vector<float>(begin + angle_to_index(angle_left_window_start_),
                         begin + angle_to_index(angle_left_window_end_)),
      distance_lateral_threshold_);
  const auto now = this->now();
  if (!front_wall_detected && !left_wall_detected) {
    if (!first_time_exited_detected_) {
      first_time_exited_detected_ = std::make_unique<rclcpp::Time>(now);
    } else {
      if (now - *first_time_exited_detected_ > exit_detection_duration_) {
        exited_ = true;
        RCLCPP_INFO(this->get_logger(), "Exit detected, stopping the robot.");
        twist.linear.x = 0.0;
        twist.angular.z = 0.0;
      }
    }
  } else{
    first_time_exited_detected_.reset();
  }
  return twist;
}

void fuga_agents::WallFollower::check_angle_ranges(const float angle_min,
                                                   const float angle_max,
                                                   float &angle) const {
  if (angle < angle_min)
    angle = angle_min;
  else if (angle > angle_max)
    angle = angle_max;
}

bool fuga_agents::WallFollower::is_wall_detected(
    const std::vector<float> &distances, float distance_threshold) {
  return median(distances) < distance_threshold;
}

// wall_follower.cpp
float fuga_agents::WallFollower::median(const std::vector<float> &distances) {
  std::vector<float> valid;
  valid.reserve(distances.size());
  for (float d : distances) {
    if (std::isfinite(d))
      valid.push_back(d);
  }
  if (valid.empty())
    return std::numeric_limits<float>::infinity();

  auto mid = valid.begin() + valid.size() / 2;
  std::nth_element(valid.begin(), mid, valid.end());
  return *mid;
}