#pragma once
#include <cmath>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp/subscription.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"

namespace fuga_agents {
class WallFollower : public rclcpp::Node {
public:
  WallFollower();
  geometry_msgs::msg::Twist
  decide(const sensor_msgs::msg::LaserScan::SharedPtr msg);

private:
  void check_angle_ranges(const float angle_min, const float angle_max,
                          float &angle) const;

  bool is_wall_detected(const std::vector<float> &distances,
                        float distance_threshold);
  float median(const std::vector<float> &distances);

  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr subscription_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;

  std::unique_ptr<rclcpp::Time> first_time_exited_detected_;
  bool exited_ = false; // Flag to indicate if the exit has been detected
  const rclcpp::Duration exit_detection_duration_ =
      rclcpp::Duration::from_seconds(5.0); // Duration to confirm exit detection

  // Maze geometry, must match kCell and T in maze_gen/export.hpp
  static constexpr float cell_size_ = 1.0;      // (m)
  static constexpr float wall_thickness_ = 0.1; // (m)
  // Robot half width: body_y / 2 + wheel_w, from fuga.urdf.xacro
  static constexpr float robot_half_width_ = 0.16 / 2 + 0.02; // (m)

  static constexpr float velocity_ = 0.3; // Velocity of the robot
  static constexpr float corner_velocity_ =
      velocity_ / 2; // Forward speed while turning around an outer corner
  static constexpr float kp_ = 3.0; // Proportional gain for wall following
  static constexpr float kd_ = 1.5; // Derivative gain for wall following
  // Target distance to the wall: the centre of the corridor
  static constexpr float distance_target_ = (cell_size_ - wall_thickness_) / 2;
  static constexpr float turn_speed_ =
      1.0; // Speed of turning when a wall is detected
  static constexpr float max_angular_ = 1.5; // Maximum angular speed (rad/s)

  // right wall
  static constexpr float angle_right_ = -M_PI / 2;
  static constexpr float window_angle_right_ = 5 * M_PI / 180; // 5 degree
  static constexpr float angle_right_window_start_ =
      angle_right_ - window_angle_right_ / 2;
  static constexpr float angle_right_window_end_ =
      angle_right_ + window_angle_right_ / 2;

  // front right wall
  static constexpr float angle_delta_ = 45 * M_PI / 180; // 45 degree
  static constexpr float angle_front_right_ = angle_right_ + angle_delta_;
  static constexpr float window_angle_front_right_ = window_angle_right_;
  static constexpr float angle_front_right_window_start_ =
      angle_front_right_ - window_angle_front_right_ / 2;
  static constexpr float angle_front_right_window_end_ =
      angle_front_right_ + window_angle_front_right_ / 2;

  // left wall
  static constexpr float angle_left_ = M_PI / 2;
  static constexpr float window_angle_left_ = window_angle_right_;
  static constexpr float angle_left_window_start_ =
      angle_left_ - window_angle_left_ / 2;
  static constexpr float angle_left_window_end_ =
      angle_left_ + window_angle_left_ / 2;

  // Thresholds to detect a wall
  // Margin so a wall at exactly distance_target_ (e.g. a dead end after
  // turning) still counts as a wall
  static constexpr float wall_margin_ = 0.05;
  // Turn at about the cell centre so the robot ends centred in the new corridor
  static constexpr float distance_front_threshold_ =
      distance_target_ + wall_margin_;
  // Halfway between a wall at distance_target_ and the wall seen through an
  // opening, one cell further (distance_target_ + cell_size_)
  static constexpr float distance_lateral_threshold_ =
      distance_target_ + cell_size_ / 2;
  const float distance_front_right_threshold_ =
      distance_lateral_threshold_ / std::cos(angle_delta_);

  // front wall
  static constexpr float angle_front_ = 0;
  // Wide enough to cover the robot's width at the front threshold, so a wall
  // that would hit a side of the robot is also detected
  const float window_angle_front_ =
      2 * std::atan(robot_half_width_ / distance_front_threshold_);
  const float angle_front_window_start_ =
      angle_front_ - window_angle_front_ / 2;
  const float angle_front_window_end_ = angle_front_ + window_angle_front_ / 2;
};
} // namespace fuga_agents