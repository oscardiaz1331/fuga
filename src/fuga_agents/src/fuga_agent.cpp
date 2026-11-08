#include "fuga_agents/fuga_agent.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <tf2/LinearMath/Quaternion.hpp>
#include <tf2/LinearMath/Transform.hpp>
#include <tf2/LinearMath/Vector3.hpp>
#include <tf2/utils.hpp>

namespace fuga_agents
{
FugaAgent::FugaAgent(const std::string & name)
: rclcpp::Node(name),
  tf_exit_buffer_(std::make_unique<tf2_ros::Buffer>(this->get_clock())),
  tf_exit_listener_(std::make_unique<tf2_ros::TransformListener>(*tf_exit_buffer_)),
  cmd_exit_vel_(this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10))
{
}

std::optional<tf2::Transform> FugaAgent::lookup(const std::string & frame, double max_age) const
{
  geometry_msgs::msg::TransformStamped tf;
  try {
    tf = tf_exit_buffer_->lookupTransform("odom", frame, tf2::TimePointZero);
  } catch (const tf2::TransformException &) {
    return std::nullopt;  // aún no se ha visto nunca
  }
  const rclcpp::Time stamp(tf.header.stamp, get_clock()->get_clock_type());
  if ((now() - stamp).seconds() > max_age) return std::nullopt;  // TF demasiado antiguo
  const auto & t = tf.transform.translation;
  const auto & q = tf.transform.rotation;
  return tf2::Transform({q.x, q.y, q.z, q.w}, {t.x, t.y, t.z});
}

std::optional<tf2::Transform> FugaAgent::check_exit_tag_transform() const
{
  // El detector solo publica el TF en las imágenes en que ve el tag, y el buffer
  // guarda el último unos segundos: hay que mirar que sea reciente
  return lookup("exit_tag", kTagMaxAge);
}

void FugaAgent::step()
{
  if (!is_going_to_exit_) {
    const auto tag = check_exit_tag_transform();
    if (tag) is_going_to_exit_ = start_exit(*tag);
    if (!is_going_to_exit_) {
      do_step();
      return;
    }
  }
  // Una vez visto el tag ya no se llama a do_step(): solo se conduce hacia la salida,
  // aunque el tag deje de verse al acercarse
  drive_exit();
}

bool FugaAgent::start_exit(const tf2::Transform & tag)
{
  const auto robot = lookup("base_link", 1.0);
  if (!robot) return false;
  const tf2::Vector3 door(tag.getOrigin().x(), tag.getOrigin().y(), 0.0);
  const tf2::Vector3 to_door =
    door - tf2::Vector3(robot->getOrigin().x(), robot->getOrigin().y(), 0.0);
  if (to_door.length() > kMaxTagDistance) return false;  // aún lejos: sigue con do_step

  // Columna 2 de la rotación = eje z del tag en odom, perpendicular al panel.
  // Su sentido cambia según el detector (y el panel se ve por las dos caras), así
  // que se elige el que se aleja del robot: hacia fuera del laberinto
  tf2::Vector3 out = tag.getBasis().getColumn(2);
  out.setZ(0.0);
  if (out.length() < 1e-3) return false;
  out.normalize();
  if (out.dot(to_door) < 0.0) out = -out;

  exit_waypoints_ = {door - out * kExitApproach, door + out * kExitLeave};
  // Si ya está entre el punto de entrada y el hueco, no da la vuelta para ir a él
  if (out.dot(to_door) < kExitApproach) exit_waypoints_.erase(exit_waypoints_.begin());

  RCLCPP_INFO(get_logger(), "Tag de salida visto en (%.2f, %.2f): salgo", door.x(), door.y());
  on_exit_start();
  return true;
}

void FugaAgent::drive_exit()
{
  if (exit_waypoints_.empty()) return;  // ya ha salido
  const auto robot = lookup("base_link", 1.0);
  if (!robot) return;
  const tf2::Vector3 & goal = exit_waypoints_.front();
  const double dx = goal.x() - robot->getOrigin().x();
  const double dy = goal.y() - robot->getOrigin().y();
  const double dist = std::hypot(dx, dy);

  if (dist < kWaypointTolerance) {
    exit_waypoints_.erase(exit_waypoints_.begin());
    if (exit_waypoints_.empty()) {
      cmd_exit_vel_->publish(geometry_msgs::msg::Twist{});
      RCLCPP_INFO(get_logger(), "Fuera del laberinto");
    }
    return;
  }

  const double error =
    std::remainder(std::atan2(dy, dx) - tf2::getYaw(robot->getRotation()), 2.0 * M_PI);
  geometry_msgs::msg::Twist cmd;
  cmd.angular.z = std::clamp(kTurnGain * error, -kMaxTurn, kMaxTurn);
  // Primero gira y luego avanza: así entra recto por el hueco
  if (std::abs(error) < kHeadingTolerance) cmd.linear.x = std::min(kExitSpeed, dist);
  cmd_exit_vel_->publish(cmd);
}
}  // namespace fuga_agents
