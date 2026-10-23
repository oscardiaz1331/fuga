#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/node.hpp"
#include "rclcpp/parameter.hpp"
#include "rclcpp/subscription.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"

namespace fuga_agents {
class WallFollower : public rclcpp::Node {
public:
  WallFollower();
  geometry_msgs::msg::Twist
  decide(const sensor_msgs::msg::LaserScan::SharedPtr msg);

private:
  // ---- Parameter names: the only place where they are written ----
  static constexpr std::string_view kSide = "side"; // kSideRight | kSideLeft
  static constexpr std::string_view kVelocity = "velocity";
  static constexpr std::string_view kCornerVelocityRatio =
      "corner_velocity_ratio";
  static constexpr std::string_view kKp = "kp";
  static constexpr std::string_view kKd = "kd";
  static constexpr std::string_view kTurnSpeed = "turn_speed";
  static constexpr std::string_view kMaxAngular = "max_angular";
  static constexpr std::string_view kCellSize = "cell_size";
  static constexpr std::string_view kWallThickness = "wall_thickness";
  static constexpr std::string_view kRobotHalfWidth = "robot_half_width";
  static constexpr std::string_view kDistanceTarget = "distance_target";
  static constexpr std::string_view kWallMargin = "wall_margin";
  static constexpr std::string_view kSideWindow = "side_window";
  static constexpr std::string_view kDiagonalAngle = "diagonal_angle";
  static constexpr std::string_view kExitTimeout = "exit_timeout";

  static constexpr std::string_view kSideRight = "right";
  static constexpr std::string_view kSideLeft = "left";

  // FNV-1a. C++ can't switch on strings, so we switch on the hash of the
  // name. Two names with the same hash would be a duplicate case label,
  // which is a compile error, so collisions between our names are caught.
  static constexpr std::uint64_t hash(std::string_view s) {
    std::uint64_t h = 14695981039346656037ull;
    for (const char c : s) {
      h ^= static_cast<unsigned char>(c);
      h *= 1099511628211ull;
    }
    return h;
  }

  // Values as set through ROS parameters. Angles in rad, distances in m,
  // speeds in m/s and rad/s, time in s.
  struct Params {
    std::string side;
    float velocity;
    float corner_velocity_ratio; // Forward speed around outer corners
    float kp;                    // Proportional gain for wall following
    float kd;                    // Derivative gain for wall following
    float turn_speed;            // Turn in place when a wall is in front
    float max_angular;
    // Maze geometry, defaults match kCell and T in maze_gen/export.hpp
    float cell_size;
    float wall_thickness;
    // body_y / 2 + wheel_w, from fuga.urdf.xacro
    float robot_half_width;
    float distance_target; // Desired distance to the followed wall
    // Margin so a wall at exactly distance_target (e.g. a dead end after
    // turning) still counts as a wall
    float wall_margin;
    float side_window;    // Width of the wall, diagonal and opposite windows
    float diagonal_angle; // From the followed side towards the front
    float exit_timeout;   // Time with no walls around to confirm the exit
  };

  // Angular range of laser rays (rad)
  struct Window {
    float start;
    float end;
  };

  // Everything computed from Params. Recomputed only when a parameter
  // changes, so decide() never repeats this work.
  struct Derived {
    // Sign of the followed wall's y in base_link: +1 left, -1 right.
    // Rotating towards the wall is +side_sign, away from it is -side_sign.
    float side_sign;
    float corner_velocity;
    float front_threshold;    // Turn at about the cell centre
    float lateral_threshold;  // Halfway between a wall and a gap
    float diagonal_threshold; // lateral_threshold seen at diagonal_angle
    Window wall;
    Window diagonal;
    Window opposite;
    Window front; // Covers the robot's width at front_threshold
  };

  std::vector<rclcpp::Parameter> declare_params();
  static void set_field(Params &params, const rclcpp::Parameter &param);
  static std::string validate(const Params &params); // Empty if valid
  static Derived derive(const Params &params);

  void check_angle_ranges(const float angle_min, const float angle_max,
                          float &angle) const;

  bool is_wall_detected(const std::vector<float> &distances,
                        float distance_threshold);
  float median(const std::vector<float> &distances);

  Params params_{};
  Derived derived_{};

  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr subscription_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;

  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr
      on_set_handle_;
  rclcpp::node_interfaces::PostSetParametersCallbackHandle::SharedPtr
      post_set_handle_;

  std::unique_ptr<rclcpp::Time> first_time_exited_detected_;
  bool exited_ = false; // Flag to indicate if the exit has been detected
};
} // namespace fuga_agents
