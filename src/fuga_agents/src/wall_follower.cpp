#include "fuga_agents/wall_follower.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <rclcpp/create_subscription.hpp>
#include <stdexcept>

namespace {
constexpr double kPi = std::numbers::pi;
constexpr double kDeg = kPi / 180.0;
} // namespace

fuga_agents::WallFollower::WallFollower() : rclcpp::Node("wall_follower") {
  // Values from a launch file or YAML don't go through the callbacks below,
  // so they are validated here
  for (const auto &param : declare_params())
    set_field(params_, param);
  if (const std::string error = validate(params_); !error.empty())
    throw std::invalid_argument(error);
  derived_ = derive(params_);

  // Runs before the change is accepted: validate the parameters as they would
  // be after the change, so checks between parameters see all new values
  on_set_handle_ = add_on_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter> &params) {
        Params candidate = params_;
        for (const auto &param : params)
          set_field(candidate, param);
        rcl_interfaces::msg::SetParametersResult result;
        result.reason = validate(candidate);
        result.successful = result.reason.empty();
        return result;
      });
  // Runs once the change is accepted
  post_set_handle_ = add_post_set_parameters_callback(
      [this](const std::vector<rclcpp::Parameter> &params) {
        for (const auto &param : params)
          set_field(params_, param);
        derived_ = derive(params_);
      });

  publisher_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
  auto callback = [this](const sensor_msgs::msg::LaserScan::SharedPtr msg) {
    this->publisher_->publish(this->decide(msg));
  };
  subscription_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
      "scan", 10, callback);
}

std::vector<rclcpp::Parameter> fuga_agents::WallFollower::declare_params() {
  std::vector<rclcpp::Parameter> declared;
  // Declares a parameter and keeps its value (the default, or the one
  // given at launch)
  const auto declare = [this, &declared](
                           std::string_view name,
                           const rclcpp::ParameterValue &default_value,
                           const rcl_interfaces::msg::ParameterDescriptor
                               &descriptor) -> const rclcpp::ParameterValue & {
    const std::string name_str{name};
    declared.emplace_back(
        name_str, declare_parameter(name_str, default_value, descriptor));
    return declared.back().get_parameter_value();
  };
  // ROS rejects values outside [from, to] before calling our callbacks
  const auto range = [](std::string_view description, double from,
                        double to) {
    rcl_interfaces::msg::ParameterDescriptor descriptor;
    descriptor.description = description;
    rcl_interfaces::msg::FloatingPointRange range;
    range.from_value = from;
    range.to_value = to;
    range.step = 0.0; // Continuous
    descriptor.floating_point_range.push_back(range);
    return descriptor;
  };

  rcl_interfaces::msg::ParameterDescriptor side;
  side.description = "Wall to follow";
  side.additional_constraints = "'right' or 'left'";
  declare(kSide, rclcpp::ParameterValue(std::string{kSideRight}), side);

  declare(kVelocity, rclcpp::ParameterValue(0.3),
          range("Forward speed while following the wall (m/s)", 0.0, 2.0));
  declare(kCornerVelocityRatio, rclcpp::ParameterValue(0.5),
          range("Fraction of velocity used around outer corners", 0.0, 1.0));
  declare(kKp, rclcpp::ParameterValue(3.0),
          range("Proportional gain for wall following", 0.0, 20.0));
  declare(kKd, rclcpp::ParameterValue(1.5),
          range("Derivative gain for wall following", 0.0, 20.0));
  declare(kTurnSpeed, rclcpp::ParameterValue(1.0),
          range("Turn speed in place when a wall is in front (rad/s)", 0.0,
                5.0));
  declare(kMaxAngular, rclcpp::ParameterValue(1.5),
          range("Maximum angular speed while following (rad/s)", 0.0, 5.0));

  const double cell_size =
      declare(kCellSize, rclcpp::ParameterValue(1.0),
              range("Maze cell size, kCell in maze_gen (m)", 0.1, 10.0))
          .get<double>();
  const double wall_thickness =
      declare(kWallThickness, rclcpp::ParameterValue(0.1),
              range("Maze wall thickness, T in maze_gen (m)", 0.0, 1.0))
          .get<double>();
  declare(kRobotHalfWidth, rclcpp::ParameterValue(0.16 / 2 + 0.02),
          range("Robot half width, body_y / 2 + wheel_w (m)", 0.01, 1.0));
  // Default: the centre of the corridor
  declare(kDistanceTarget,
          rclcpp::ParameterValue((cell_size - wall_thickness) / 2),
          range("Desired distance to the followed wall (m)", 0.01, 5.0));
  declare(kWallMargin, rclcpp::ParameterValue(0.05),
          range("Extra distance at which a front wall still counts (m)", 0.0,
                1.0));

  // The lidar has one ray per degree, so the window needs at least 2 degrees
  declare(kSideWindow, rclcpp::ParameterValue(5 * kDeg),
          range("Width of the side, diagonal and opposite windows (rad)",
                2 * kDeg, kPi / 2));
  declare(kDiagonalAngle, rclcpp::ParameterValue(45 * kDeg),
          range("Angle from the followed side towards the front (rad)",
                10 * kDeg, 80 * kDeg));

  declare(kExitTimeout, rclcpp::ParameterValue(30.0),
          range("Time with no walls in front or opposite to stop (s)", 0.0,
                600.0));
  return declared;
}

void fuga_agents::WallFollower::set_field(Params &params,
                                          const rclcpp::Parameter &param) {
  // ROS has already checked the type, since every parameter is declared
  // with one
  const auto value = [&param] { return static_cast<float>(param.as_double()); };
  switch (hash(param.get_name())) {
  case hash(kSide):
    params.side = param.as_string();
    break;
  case hash(kVelocity):
    params.velocity = value();
    break;
  case hash(kCornerVelocityRatio):
    params.corner_velocity_ratio = value();
    break;
  case hash(kKp):
    params.kp = value();
    break;
  case hash(kKd):
    params.kd = value();
    break;
  case hash(kTurnSpeed):
    params.turn_speed = value();
    break;
  case hash(kMaxAngular):
    params.max_angular = value();
    break;
  case hash(kCellSize):
    params.cell_size = value();
    break;
  case hash(kWallThickness):
    params.wall_thickness = value();
    break;
  case hash(kRobotHalfWidth):
    params.robot_half_width = value();
    break;
  case hash(kDistanceTarget):
    params.distance_target = value();
    break;
  case hash(kWallMargin):
    params.wall_margin = value();
    break;
  case hash(kSideWindow):
    params.side_window = value();
    break;
  case hash(kDiagonalAngle):
    params.diagonal_angle = value();
    break;
  case hash(kExitTimeout):
    params.exit_timeout = value();
    break;
  default: // Not ours, e.g. use_sim_time
    break;
  }
}

// Only what the ranges in declare_params() can't express
std::string fuga_agents::WallFollower::validate(const Params &params) {
  if (params.side != kSideRight && params.side != kSideLeft)
    return "side must be 'right' or 'left'";
  if (params.wall_thickness >= params.cell_size)
    return "wall_thickness must be smaller than cell_size";
  // The robot must not touch either wall of the corridor
  const float corridor = params.cell_size - params.wall_thickness;
  if (params.distance_target <= params.robot_half_width ||
      params.distance_target >= corridor - params.robot_half_width)
    return "distance_target must be between robot_half_width and "
           "cell_size - wall_thickness - robot_half_width";
  return {};
}

fuga_agents::WallFollower::Derived
fuga_agents::WallFollower::derive(const Params &params) {
  constexpr float kHalfPi = std::numbers::pi_v<float> / 2;
  const auto window = [](float centre, float width) {
    return Window{centre - width / 2, centre + width / 2};
  };

  Derived derived{};
  derived.side_sign = params.side == kSideLeft ? 1.0f : -1.0f;
  derived.corner_velocity = params.velocity * params.corner_velocity_ratio;
  // Turn at about the cell centre so the robot ends centred in the new
  // corridor
  derived.front_threshold = params.distance_target + params.wall_margin;
  // Halfway between a wall at distance_target and the wall seen through an
  // opening, one cell further (distance_target + cell_size)
  derived.lateral_threshold = params.distance_target + params.cell_size / 2;
  derived.diagonal_threshold =
      derived.lateral_threshold / std::cos(params.diagonal_angle);

  derived.wall = window(derived.side_sign * kHalfPi, params.side_window);
  derived.diagonal = window(
      derived.side_sign * (kHalfPi - params.diagonal_angle), params.side_window);
  derived.opposite = window(-derived.side_sign * kHalfPi, params.side_window);
  // Wide enough to cover the robot's width at the front threshold, so a wall
  // that would hit a side of the robot is also detected
  derived.front =
      window(0.0f, 2 * std::atan(params.robot_half_width /
                                 derived.front_threshold));
  return derived;
}

geometry_msgs::msg::Twist fuga_agents::WallFollower::decide(
    const sensor_msgs::msg::LaserScan::SharedPtr msg) {
  if (exited_) {
    geometry_msgs::msg::Twist twist;
    twist.linear.x = 0.0;
    twist.angular.z = 0.0;
    return twist;
  }
  const Params &p = params_;
  const Derived &d = derived_;

  const float angle_min = msg->angle_min;
  const float angle_max = msg->angle_max;
  const float angle_increment = msg->angle_increment;
  const auto begin = msg->ranges.begin();
  const auto angle_to_index = [angle_min, angle_max, angle_increment,
                               this](float angle) {
    check_angle_ranges(angle_min, angle_max, angle);
    return static_cast<int>((angle - angle_min) / angle_increment);
  };
  const auto rays = [&](const Window &window) {
    return std::vector<float>(begin + angle_to_index(window.start),
                              begin + angle_to_index(window.end));
  };

  const float distance_wall = median(rays(d.wall));
  const float distance_diagonal = median(rays(d.diagonal));
  const bool front_wall_detected =
      is_wall_detected(rays(d.front), d.front_threshold);
  const bool side_wall_detected = distance_wall < d.lateral_threshold;
  // Twist
  geometry_msgs::msg::Twist twist;

  if (front_wall_detected) {
    // 1. Esquina interior / callejón: girar en el sitio alejándose de la pared
    twist.linear.x = 0.0;
    twist.angular.z = -d.side_sign * p.turn_speed;
  } else if (!side_wall_detected) {
    // 2. Esquina exterior: curva hacia la pared con radio = distance_target
    twist.linear.x = d.corner_velocity;
    twist.angular.z = d.side_sign * d.corner_velocity / p.distance_target;
  } else {
    // 3. Seguir la pared con el PD
    float angle = 0.0f; // sin lectura fiable en diagonal, ir recto
    if (std::isfinite(distance_diagonal) &&
        distance_diagonal < d.diagonal_threshold) {
      angle = std::atan2(distance_diagonal * std::cos(p.diagonal_angle) -
                             distance_wall,
                         distance_diagonal * std::sin(p.diagonal_angle));
    }
    const float distance_to_wall = distance_wall * std::cos(angle);
    // Positivo = alejarse de la pared
    const float correction =
        -p.kp * (distance_to_wall - p.distance_target) - p.kd * angle;
    twist.linear.x = p.velocity;
    twist.angular.z = std::clamp(-d.side_sign * correction, -p.max_angular,
                                 p.max_angular);
  }

  const bool opposite_wall_detected =
      is_wall_detected(rays(d.opposite), d.lateral_threshold);
  const auto now = this->now();
  if (!front_wall_detected && !opposite_wall_detected) {
    if (!first_time_exited_detected_) {
      first_time_exited_detected_ = std::make_unique<rclcpp::Time>(now);
    } else {
      if ((now - *first_time_exited_detected_).seconds() > p.exit_timeout) {
        exited_ = true;
        RCLCPP_INFO(this->get_logger(), "Exit detected, stopping the robot.");
        twist.linear.x = 0.0;
        twist.angular.z = 0.0;
      }
    }
  } else {
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
