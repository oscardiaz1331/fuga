#include "fuga_agents/frontier_explorer.hpp"

#include <algorithm>
#include <cmath>

#include "fuga_agents/frontiers.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"

namespace fuga_agents
{
namespace
{
// Centro de la celda cell en metros
double cell_to_meters(double origin, int cell, double resolution)
{
  return origin + (static_cast<double>(cell) + 0.5) * resolution;
}

// floor y no static_cast: -0.5 debe dar -1 (fuera del grid) y no 0
int meters_to_cell(double origin, double meters, double resolution)
{
  return static_cast<int>(std::floor((meters - origin) / resolution));
}

template<typename P>
bool near_any(const std::vector<P> & points, double x, double y, double radius)
{
  return std::any_of(points.begin(), points.end(), [&](const P & p) {
             return std::hypot(x - p.x, y - p.y) < radius;
           });
}
}  // namespace

FrontierExplorer::FrontierExplorer()
: FugaAgent("frontier_explorer"),
  navigator_(std::make_shared<fuga_cpp_navigate_to_pose::NavigateToPoseCpp>(*this)),
  tf_buffer_(std::make_unique<tf2_ros::Buffer>(this->get_clock())),
  tf_listener_(std::make_unique<tf2_ros::TransformListener>(*tf_buffer_))
{
  search_exits_ = declare_parameter("search_exits", false);
  require_exit_tag_ = declare_parameter("require_exit_tag", true);
  exit_tag_frame_ = declare_parameter("exit_tag_frame", std::string("exit_tag"));

  subscription_ = this->create_subscription<nav_msgs::msg::OccupancyGrid>(
    "map", 10, [this](const nav_msgs::msg::OccupancyGrid::SharedPtr msg) { map_callback(msg); });
  cmd_vel_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
  // create_timer usa el reloj del nodo (no create_wall_timer): con use_sim_time son
  // 200 ms simulados, así decide al mismo ritmo aunque la simulación vaya acelerada.
  // Hasta que llega el primer /clock el tiempo vale 0 y el timer no salta.
  // 200 ms y no más: es lo que el robot puede quedarse parado al llegar a un goal
  // antes de recibir el siguiente. Cada tick es barato mientras navega (is_still_frontier
  // y find_exits); find_frontiers y el BFS solo se ejecutan al elegir.
  using namespace std::chrono_literals;
  timer_ = this->create_timer(200ms, [this]() { step(); });
}

void FrontierExplorer::on_exit_start()
{
  navigator_->cancel_goal();
  current_goal_result_ = {};
  mode_ = Mode::kDone;
}

void FrontierExplorer::map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
  last_map_ = msg;
}

void FrontierExplorer::do_step()
{
  // El timer puede saltar antes de que SLAM publique el primer mapa
  if (!last_map_) return;
  switch (mode_) {
    case Mode::kExplore:
      explore();
      break;
    case Mode::kApproachExit:
      approach_exit();
      break;
    case Mode::kFaceExit:
      face_exit();
      break;
    case Mode::kLeave:
      leave();
      break;
    case Mode::kDone:
      break;
  }
}

void FrontierExplorer::explore()
{
  using namespace std::chrono_literals;
  // Salir tiene prioridad sobre seguir explorando. El goal de la frontera actual lo
  // sustituye el de la salida, sin frenar
  if (search_exits_ && start_exit()) return;

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
  const std::optional<Pose2D> robot = robot_pose();
  if (!robot) return GoalChoice::kNoPose;

  // Encontrar fronteras y elegir una
  const auto & info = last_map_->info;
  const double origin_x = info.origin.position.x;
  const double origin_y = info.origin.position.y;

  // Holgura del objetivo: sin ella, el objetivo podía caer pegado a un muro (dentro del
  // radio del robot) y Nav2 no llegaba nunca ("Failed to make progress")
  const int clearance = static_cast<int>(std::ceil(kTargetClearance / info.resolution));
  const auto frontiers =
    find_frontiers(info.width, info.height, last_map_->data, kMinFrontierSize, clearance);

  std::vector<Frontier> candidates;
  for (const auto & f : frontiers) {
    const double fx = cell_to_meters(origin_x, f.target_x, info.resolution);
    const double fy = cell_to_meters(origin_y, f.target_y, info.resolution);
    if (!near_any(failed_targets_, fx, fy, kFailedRadius)) {
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
    info.width, info.height, last_map_->data, meters_to_cell(origin_x, robot->x, info.resolution),
    meters_to_cell(origin_y, robot->y, info.resolution));
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
  const Point target{
    cell_to_meters(origin_x, frontier.target_x, info.resolution),
    cell_to_meters(origin_y, frontier.target_y, info.resolution)};
  // Orientación: la dirección del robot al objetivo. Nav2 ya no la exige al llegar
  // (yaw_goal_tolerance 3.14 y GoalAngleCritic desactivado en nav2_params.yaml), pero
  // si se volviera a exigir, así no giraría en cada frontera para mirar al eje x
  const double yaw = std::atan2(target.y - robot->y, target.x - robot->x);
  RCLCPP_INFO(get_logger(), "Frontera elegida: (%.2f, %.2f) m", target.x, target.y);
  current_target_ = target;
  current_goal_result_ = navigator_->send_goal(make_goal(target, yaw));
  return GoalChoice::kSent;
}

bool FrontierExplorer::start_exit()
{
  const std::optional<Pose2D> robot = robot_pose();
  if (!robot) return false;

  const auto & info = last_map_->info;
  std::optional<Exit> best;
  double best_distance = 0.0;
  for (const ExitCandidate & c :
    find_exits(info.width, info.height, last_map_->data, kMinFrontierSize))
  {
    const Point door{
      cell_to_meters(info.origin.position.x, c.door_x, info.resolution),
      cell_to_meters(info.origin.position.y, c.door_y, info.resolution)};
    if (near_any(rejected_exits_, door.x, door.y, kFailedRadius)) continue;
    const double distance = std::hypot(door.x - robot->x, door.y - robot->y);
    if (!best || distance < best_distance) {
      best = Exit{door, {static_cast<double>(c.out_dx), static_cast<double>(c.out_dy)}};
      best_distance = distance;
    }
  }
  if (!best) return false;

  // El goal de frontera que hubiera en curso lo sustituye el nuevo; su future se descarta
  exit_ = best;
  current_target_.reset();
  replanning_ = false;
  if (!require_exit_tag_) {
    RCLCPP_INFO(get_logger(), "Salida en (%.2f, %.2f) m: salgo", exit_->door.x, exit_->door.y);
    send_leave_goal();
    return true;
  }
  RCLCPP_INFO(
    get_logger(), "Salida en (%.2f, %.2f) m: voy a comprobar el tag", exit_->door.x,
    exit_->door.y);
  // Nav2 no exige la orientación al llegar (yaw_goal_tolerance 3.14): la corrige face_exit()
  const Point approach{
    exit_->door.x - exit_->out.x * kExitApproach, exit_->door.y - exit_->out.y * kExitApproach};
  current_goal_result_ =
    navigator_->send_goal(make_goal(approach, std::atan2(exit_->out.y, exit_->out.x)));
  mode_ = Mode::kApproachExit;
  return true;
}

void FrontierExplorer::approach_exit()
{
  using namespace std::chrono_literals;
  // Si el tag se ve ya por el camino, no hace falta pararse a girar
  if (tag_seen_near(exit_->door)) {
    RCLCPP_INFO(get_logger(), "Tag de la salida visto: salgo");
    send_leave_goal();
    return;
  }
  if (current_goal_result_.wait_for(0s) != std::future_status::ready) return;
  const rclcpp_action::ResultCode code = current_goal_result_.get();
  current_goal_result_ = {};
  if (code != rclcpp_action::ResultCode::SUCCEEDED) {
    reject_exit("Nav2 no llega delante de la salida");
    return;
  }
  mode_ = Mode::kFaceExit;
  face_start_ = now();
}

void FrontierExplorer::face_exit()
{
  if (tag_seen_near(exit_->door)) {
    RCLCPP_INFO(get_logger(), "Tag de la salida visto: salgo");
    stop_turning();
    send_leave_goal();
    return;
  }
  if ((now() - face_start_).seconds() > kFaceTimeout) {
    stop_turning();
    reject_exit("No se ve el tag");
    return;
  }
  const std::optional<Pose2D> robot = robot_pose();
  if (!robot) return;
  // Hacia el centro del hueco, no solo hacia fuera: Nav2 deja el robot a hasta
  // xy_goal_tolerance del objetivo, también de lado, y así el tag queda centrado
  const double target = std::atan2(exit_->door.y - robot->y, exit_->door.x - robot->x);
  const double error = std::remainder(target - robot->yaw, 2.0 * M_PI);  // en [-pi, pi]
  geometry_msgs::msg::Twist cmd;
  if (std::abs(error) > kFaceTolerance) {
    cmd.angular.z = std::clamp(kTurnGain * error, -kMaxTurnSpeed, kMaxTurnSpeed);
  }
  // Ya orientado se publica 0: el robot espera quieto a que apriltag_ros procese una imagen
  cmd_vel_->publish(cmd);
}

void FrontierExplorer::leave()
{
  using namespace std::chrono_literals;
  if (current_goal_result_.wait_for(0s) != std::future_status::ready) return;
  const rclcpp_action::ResultCode code = current_goal_result_.get();
  current_goal_result_ = {};
  if (code == rclcpp_action::ResultCode::SUCCEEDED) {
    RCLCPP_INFO(get_logger(), "Fuera del laberinto");
    mode_ = Mode::kDone;
    return;
  }
  // El tag ya confirmó que es una salida: merece la pena reintentar antes de descartarla
  if (++exit_->leave_attempts < kMaxLeaveAttempts) {
    RCLCPP_WARN(
      get_logger(), "Nav2 no ha podido salir (intento %d de %d): reintento",
      exit_->leave_attempts, kMaxLeaveAttempts);
    send_leave_goal();
    return;
  }
  reject_exit("Nav2 no consigue salir");
}

void FrontierExplorer::send_leave_goal()
{
  const Point outside{
    exit_->door.x + exit_->out.x * kExitLeave, exit_->door.y + exit_->out.y * kExitLeave};
  current_goal_result_ =
    navigator_->send_goal(make_goal(outside, std::atan2(exit_->out.y, exit_->out.x)));
  mode_ = Mode::kLeave;
}

void FrontierExplorer::reject_exit(const char * reason)
{
  RCLCPP_WARN(
    get_logger(), "%s: salida (%.2f, %.2f) descartada, sigo explorando", reason, exit_->door.x,
    exit_->door.y);
  rejected_exits_.push_back(exit_->door);
  exit_.reset();
  mode_ = Mode::kExplore;
}

bool FrontierExplorer::tag_seen_near(const Point & door) const
{
  // apriltag_ros solo publica el TF del tag en las imágenes en que lo detecta, y el
  // buffer guarda el último unos segundos: hay que mirar que sea reciente
  geometry_msgs::msg::TransformStamped tf;
  try {
    tf = tf_buffer_->lookupTransform(
      last_map_->header.frame_id, exit_tag_frame_, tf2::TimePointZero);
  } catch (const tf2::TransformException &) {
    return false;  // aún no se ha visto nunca
  }
  const rclcpp::Time stamp(tf.header.stamp, get_clock()->get_clock_type());
  if ((now() - stamp).seconds() > kTagMaxAge) return false;
  return std::hypot(tf.transform.translation.x - door.x, tf.transform.translation.y - door.y) <
         kTagDoorRadius;
}

void FrontierExplorer::stop_turning()
{
  cmd_vel_->publish(geometry_msgs::msg::Twist{});
}

std::optional<FrontierExplorer::Pose2D> FrontierExplorer::robot_pose() const
{
  geometry_msgs::msg::TransformStamped tf;
  try {
    tf = tf_buffer_->lookupTransform(last_map_->header.frame_id, "base_link", tf2::TimePointZero);
  } catch (const tf2::TransformException & e) {
    RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 5000, "Sin pose del robot: %s", e.what());
    return std::nullopt;
  }
  const auto & q = tf.transform.rotation;
  const double yaw =
    std::atan2(2.0 * (q.w * q.z + q.x * q.y), 1.0 - 2.0 * (q.y * q.y + q.z * q.z));
  return Pose2D{tf.transform.translation.x, tf.transform.translation.y, yaw};
}

geometry_msgs::msg::PoseStamped FrontierExplorer::make_goal(const Point & p, double yaw) const
{
  geometry_msgs::msg::PoseStamped pose{};
  pose.header.frame_id = last_map_->header.frame_id;
  pose.header.stamp = this->now();
  pose.pose.position.x = p.x;
  pose.pose.position.y = p.y;
  pose.pose.orientation.z = std::sin(yaw / 2.0);
  pose.pose.orientation.w = std::cos(yaw / 2.0);
  return pose;
}

bool FrontierExplorer::is_still_frontier(const Point & target) const
{
  // Se convierte en cada llamada: el origen cambia cuando SLAM agranda el mapa
  const auto & info = last_map_->info;
  const int cx = meters_to_cell(info.origin.position.x, target.x, info.resolution);
  const int cy = meters_to_cell(info.origin.position.y, target.y, info.resolution);
  const int radius = static_cast<int>(std::ceil(kTargetRadius / info.resolution));
  return has_frontier_near(info.width, info.height, last_map_->data, cx, cy, radius);
}
}  // namespace fuga_agents
