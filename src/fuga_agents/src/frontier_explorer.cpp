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
  // 200 ms simulados, así decide al mismo ritmo aunque la simulación vaya acelerada.
  // Hasta que llega el primer /clock el tiempo vale 0 y el timer no salta.
  // 200 ms y no más: es lo que el robot puede quedarse parado al llegar a un goal
  // antes de recibir el siguiente. Cada tick es barato mientras navega (solo
  // is_still_frontier); find_frontiers y el BFS solo se ejecutan al elegir.
  using namespace std::chrono_literals;
  timer_ = this->create_timer(200ms, [this]() { decide(); });
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
        // Se manda el siguiente goal directamente: Nav2 sustituye el actual sin frenar
        // el robot. Cancelar y esperar al CANCELED lo paraba en seco cada vez. El
        // future del goal sustituido (llegará ABORTED) se descarta: no es un fallo
        RCLCPP_INFO(get_logger(), "La frontera ya no existe, cambio de rumbo sin parar");
        if (send_next_goal() == GoalChoice::kNoFrontier) {
          // No queda adónde ir: parar. El future estará listo con CANCELED en un tick
          // posterior. Sin pose (kNoPose) se sigue con el goal actual y se reintenta
          RCLCPP_INFO(get_logger(), "No quedan fronteras alcanzables, parando");
          replanning_ = navigator_->cancel_goal();
        }
      }
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

  send_next_goal();
}

FrontierExplorer::GoalChoice FrontierExplorer::send_next_goal()
{
  // Obtener la pose del robot en el marco "map"
  geometry_msgs::msg::TransformStamped tf;
  try {
    tf = tf_buffer_->lookupTransform(last_map_->header.frame_id, "base_link", tf2::TimePointZero);
  } catch (const tf2::TransformException & e) {
    RCLCPP_WARN(get_logger(), "Sin pose del robot: %s", e.what());
    return GoalChoice::kNoPose;
  }

  // Encontrar fronteras y elegir una
  const auto & info = last_map_->info;
  const double origin_x = info.origin.position.x;
  const double origin_y = info.origin.position.y;

  // Holgura del objetivo: sin ella, el objetivo podía caer pegado a un muro (dentro del
  // radio del robot) y Nav2 no llegaba nunca ("Failed to make progress")
  const int clearance = static_cast<int>(std::ceil(kTargetClearance / info.resolution));
  const auto frontiers =
    find_frontiers(info.width, info.height, last_map_->data, kMinFrontierSize, clearance);

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
  // Un fallo no siempre es culpa de la frontera (timeout con la simulación lenta, Nav2
  // sin tiempo para aceptar la petición...). Si solo quedan fronteras vetadas, se
  // vuelven a intentar: el robot tiene que salir del laberinto, y quedarse parado
  // dando el mapa por explorado es peor que reintentar una que quizá falle otra vez
  if (candidates.empty() && !frontiers.empty()) {
    RCLCPP_WARN(
      get_logger(), "Solo quedan %zu fronteras vetadas por fallos anteriores: se reintentan",
      frontiers.size());
    failed_targets_.clear();
    candidates = frontiers;
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
    // Explicar por qué, para distinguir "mapa explorado" de fronteras descartadas.
    // Repetir find_frontiers sin holgura solo cuesta algo aquí, cuando ya no hay goal
    const size_t without_clearance =
      find_frontiers(info.width, info.height, last_map_->data, kMinFrontierSize, 0).size();
    RCLCPP_INFO_THROTTLE(
      get_logger(), *get_clock(), 5000,
      "No quedan fronteras alcanzables: %zu detectadas, %zu descartadas por estar "
      "pegadas a muros (sin %.2f m de holgura), %zu sin camino por celdas libres",
      without_clearance, without_clearance - frontiers.size(), kTargetClearance,
      candidates.size());
    return GoalChoice::kNoFrontier;
  }
  const Frontier & frontier = chosen_frontier.value();
  geometry_msgs::msg::PoseStamped pose{};
  pose.header.frame_id = last_map_->header.frame_id;
  pose.header.stamp = this->now();
  pose.pose.position.x = cell_to_meters(origin_x, frontier.target_x);
  pose.pose.position.y = cell_to_meters(origin_y, frontier.target_y);
  // Orientación: la dirección del robot al objetivo. Nav2 ya no la exige al llegar
  // (yaw_goal_tolerance 3.14 y GoalAngleCritic desactivado en nav2_params.yaml), pero
  // si se volviera a exigir, así no giraría en cada frontera para mirar al eje x
  // (con PoseStamped{} el yaw es 0: w = 1 por defecto)
  const double yaw = std::atan2(
    pose.pose.position.y - tf.transform.translation.y,
    pose.pose.position.x - tf.transform.translation.x);
  pose.pose.orientation.z = std::sin(yaw / 2.0);
  pose.pose.orientation.w = std::cos(yaw / 2.0);
  RCLCPP_INFO(
    get_logger(), "Frontera elegida: (%.2f, %.2f) m", pose.pose.position.x, pose.pose.position.y);
  current_target_ = Point{pose.pose.position.x, pose.pose.position.y};
  current_goal_result_ = navigator_->send_goal(pose);
  return GoalChoice::kSent;
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