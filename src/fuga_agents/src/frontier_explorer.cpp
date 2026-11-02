#include "fuga_agents/frontier_explorer.hpp"

#include <algorithm>
#include <cmath>

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
  // create_timer usa el reloj del nodo (no create_wall_timer): con use_sim_time son
  // 500 ms simulados, así decide al mismo ritmo aunque la simulación vaya acelerada.
  // Hasta que llega el primer /clock el tiempo vale 0 y el timer no salta.
  using namespace std::chrono_literals;
  timer_ = this->create_timer(500ms, [this]() { decide(); });
}

void FrontierExplorer::map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
  last_map_ = msg;
}

void FrontierExplorer::decide()
{
  using namespace std::chrono_literals;
  // El timer puede saltar antes de que SLAM publique el primer mapa
  if (!last_map_) return;
  if (current_goal_result_.valid()) {
    // Goal en curso: seguir, salvo que su frontera haya desaparecido del mapa
    if (current_goal_result_.wait_for(0s) != std::future_status::ready) {
      if (!replanning_ && current_target_ && !is_still_frontier(*current_target_)) {
        RCLCPP_INFO(get_logger(), "La frontera ya no existe, cambio de rumbo");
        replanning_ = navigator_->cancel_goal();
      }
      // No se elige otra aún: el goal sigue vivo hasta que Nav2 confirme la
      // cancelación, y en un tick posterior el future estará listo con CANCELED
      return;
    }
    if (current_goal_result_.get() != rclcpp_action::ResultCode::SUCCEEDED && !replanning_ &&
      current_target_)
    {
      RCLCPP_WARN(get_logger(), "Frontera fallida, no se volverá a intentar");
      failed_targets_.push_back(*current_target_);
    }
    replanning_ = false;
    current_goal_result_ = {};  // ya procesado: valid() pasa a false
    current_target_.reset();
  }

  // Obtener la pose del robot en el marco "map"
  geometry_msgs::msg::TransformStamped tf;
  try {
    tf = tf_buffer_->lookupTransform(last_map_->header.frame_id, "base_link", tf2::TimePointZero);
  } catch (const tf2::TransformException & e) {
    RCLCPP_WARN(get_logger(), "Sin pose del robot: %s", e.what());
    return;
  }

  // Encontrar fronteras y elegir una
  const auto & info = last_map_->info;
  const double origin_x = info.origin.position.x;
  const double origin_y = info.origin.position.y;

  const auto frontiers = find_frontiers(info.width, info.height, last_map_->data);

  const auto cell_to_meters = [&](double origin, int cell) {
    return origin + static_cast<double>(cell + 0.5) * info.resolution;
  };
  // floor y no static_cast: -0.5 debe dar -1 (fuera del grid) y no 0
  const auto meters_to_cell = [&](double origin, double meters) {
    return static_cast<int>(std::floor((meters - origin) / info.resolution));
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

  // Distancia por camino (BFS por celdas libres) desde el robot, no en línea recta:
  // en un laberinto una frontera al otro lado de una pared puede estar muy lejos
  const std::vector<int> distances = path_distances(
    info.width, info.height, last_map_->data, meters_to_cell(origin_x, tf.transform.translation.x),
    meters_to_cell(origin_y, tf.transform.translation.y));
  const std::optional<Frontier> chosen_frontier =
    choose_frontier(candidates, distances, info.width);

  // Si se encuentra una frontera, enviar un goal al servidor de acción
  if (!chosen_frontier.has_value()) {
    RCLCPP_INFO(get_logger(), "No quedan fronteras alcanzables");
    return;
  }
  const Frontier & frontier = chosen_frontier.value();
  geometry_msgs::msg::PoseStamped pose{};
  pose.header.frame_id = last_map_->header.frame_id;
  pose.header.stamp = this->now();
  pose.pose.position.x = cell_to_meters(origin_x, frontier.target_x);
  pose.pose.position.y = cell_to_meters(origin_y, frontier.target_y);
  pose.pose.orientation.w = 1.0;  // Orientación neutra
  RCLCPP_INFO(
    get_logger(), "Frontera elegida: (%.2f, %.2f) m", pose.pose.position.x, pose.pose.position.y);
  current_target_ = Point{pose.pose.position.x, pose.pose.position.y};
  current_goal_result_ = navigator_->send_goal(pose);
}

bool FrontierExplorer::is_still_frontier(const Point & target) const
{
  // Se convierte en cada llamada: el origen cambia cuando SLAM agranda el mapa.
  // floor y no static_cast: con un objetivo a la izquierda del origen, -0.5 debe
  // dar -1 (fuera del grid) y no 0
  const auto & info = last_map_->info;
  const int cx =
    static_cast<int>(std::floor((target.x - info.origin.position.x) / info.resolution));
  const int cy =
    static_cast<int>(std::floor((target.y - info.origin.position.y) / info.resolution));
  const int radius = static_cast<int>(std::ceil(kTargetRadius / info.resolution));
  return has_frontier_near(info.width, info.height, last_map_->data, cx, cy, radius);
}
}  // namespace fuga_agents