#include "fuga_agents/frontier_explorer.hpp"

#include "fuga_agents/frontiers.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"

namespace fuga_agents
{
FrontierExplorer::FrontierExplorer()
: FugaAgent("frontier_explorer"),
  navigator_(std::make_shared<fuga_cpp_navigate_to_pose::NavigateToPoseCpp>(*this)),
  tf_buffer_(std::make_unique<tf2_ros::Buffer>(this->get_clock())),
  tf_listener_(std::make_unique<tf2_ros::TransformListener>(*tf_buffer_))
{
  subscription_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
    "map", 10, [this](const nav_msgs::msg::OccupancyGrid::SharedPtr msg) { map_callback(msg); });
}
void FrontierExplorer::map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
  using namespace std::chrono_literals;
  // Si hay un goal en curso, no hacer nada
  if (current_goal_result_.valid()) {
    if (current_goal_result_.wait_for(0s) != std::future_status::ready) {
      return;  // sigue navegando
    }
    if (current_goal_result_.get() != rclcpp_action::ResultCode::SUCCEEDED && current_target_) {
      RCLCPP_WARN(get_logger(), "Frontera fallida, no se volverá a intentar");
      failed_targets_.push_back(*current_target_);
    }
    current_goal_result_ = {};  // ya procesado: valid() pasa a false
    current_target_.reset();
  }

  // Obtener la pose del robot en el marco "map"
  geometry_msgs::msg::TransformStamped tf;
  try {
    tf = tf_buffer_->lookupTransform(msg->header.frame_id, "base_link", tf2::TimePointZero);
  } catch (const tf2::TransformException & e) {
    RCLCPP_WARN(get_logger(), "Sin pose del robot: %s", e.what());
    return;
  }

  // Encontrar fronteras y elegir una
  const auto & info = msg->info;
  const double origin_x = info.origin.position.x;
  const double origin_y = info.origin.position.y;

  const auto frontiers = find_frontiers(info.width, info.height, msg->data);

  const auto cell_to_meters = [&](double origin, int cell) {
    return origin + static_cast<double>(cell + 0.5) * info.resolution;
  };
  const auto meters_to_cell = [&](double origin, double meters) {
    return static_cast<int>((meters - origin) / info.resolution);
  };

  std::vector<Frontier> candidates;
  for (const auto & f : frontiers) {
    const double fx = cell_to_meters(origin_x, f.target_x);
    const double fy = cell_to_meters(origin_y, f.target_y);
    const bool failed = std::any_of(
      failed_targets_.begin(), failed_targets_.end(),
      [&](const Point & p) { return std::hypot(fx - p.x, fy - p.y) < kFailedRadius; });
    if (!failed) {
      candidates.push_back(f);
    }
  }

  const std::optional<Frontier> chosen_frontier = choose_frontier(
    candidates, meters_to_cell(origin_x, tf.transform.translation.x),
    meters_to_cell(origin_y, tf.transform.translation.y));

  // Si se encuentra una frontera, enviar un goal al servidor de acción
  if (!chosen_frontier.has_value()) {
    RCLCPP_INFO(get_logger(), "No frontiers found.");
    return;
  }
  const Frontier & frontier = chosen_frontier.value();
  geometry_msgs::msg::PoseStamped pose{};
  pose.header.frame_id = msg->header.frame_id;
  pose.header.stamp = this->now();
  pose.pose.position.x = cell_to_meters(origin_x, frontier.target_x);
  pose.pose.position.y = cell_to_meters(origin_y, frontier.target_y);
  pose.pose.orientation.w = 1.0;  // Orientación neutra
  RCLCPP_INFO(
    get_logger(), "Frontera elegida: (%.2f, %.2f) m", pose.pose.position.x, pose.pose.position.y);
  current_target_ = Point{pose.pose.position.x, pose.pose.position.y};
  current_goal_result_ = navigator_->send_goal(pose);
}
}  // namespace fuga_agents